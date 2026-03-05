// GoodYaView.cpp : 视图类实现
//

#include "stdafx.h"
#include "GoodYa.h"

#include "GoodYaDoc.h"
#include "GoodYaView.h"
#include "ComDlg.h"
#include "DecDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// 预览编辑控件 ID
#define IDC_PREVIEW_EDIT 50001
// 固定使用 GBK 代码页（CP936）做编辑区与文档字节转换
#define GOODYA_GBK_CP 936

// 功能：读取编辑框 Unicode 内容并转换为 GBK 字节串。
static CString EditTextUnicodeToGbk(HWND hWndEdit)
{
	CString gbkText;
	if (hWndEdit == NULL)
		return gbkText;

	int nWideLen = ::GetWindowTextLengthW(hWndEdit);
	if (nWideLen <= 0)
		return gbkText;

	WCHAR* pWide = new WCHAR[nWideLen + 1];
	if (pWide == NULL)
		return gbkText;

	pWide[0] = 0;
	::GetWindowTextW(hWndEdit, pWide, nWideLen + 1);

	int nGbkLen = ::WideCharToMultiByte(GOODYA_GBK_CP, 0, pWide, nWideLen, NULL, 0, "?", NULL);
	if (nGbkLen > 0)
	{
		LPSTR pGbk = gbkText.GetBufferSetLength(nGbkLen);
		::WideCharToMultiByte(GOODYA_GBK_CP, 0, pWide, nWideLen, pGbk, nGbkLen, "?", NULL);
		gbkText.ReleaseBuffer(nGbkLen);
	}

	delete[] pWide;
	return gbkText;
}

// 功能：把 GBK 字节串转换为 Unicode 并写回编辑框显示。
static void SetEditTextFromGbk(HWND hWndEdit, const CString& gbkText)
{
	if (hWndEdit == NULL)
		return;

	int nSrcLen = gbkText.GetLength();
	if (nSrcLen <= 0)
	{
		::SetWindowTextW(hWndEdit, L"");
		return;
	}

	int nWideLen = ::MultiByteToWideChar(GOODYA_GBK_CP, 0, gbkText, nSrcLen, NULL, 0);
	if (nWideLen <= 0)
	{
		::SetWindowTextA(hWndEdit, gbkText);
		return;
	}

	WCHAR* pWide = new WCHAR[nWideLen + 1];
	if (pWide == NULL)
	{
		::SetWindowTextA(hWndEdit, gbkText);
		return;
	}

	::MultiByteToWideChar(GOODYA_GBK_CP, 0, gbkText, nSrcLen, pWide, nWideLen);
	pWide[nWideLen] = 0;
	::SetWindowTextW(hWndEdit, pWide);
	delete[] pWide;
}

// 功能：判断编辑框是否处于只读状态。
static BOOL IsEditReadOnly(HWND hWndEdit)
{
	if (hWndEdit == NULL)
		return TRUE;

	LONG lStyle = ::GetWindowLong(hWndEdit, GWL_STYLE);
	return ((lStyle & ES_READONLY) != 0);
}

// 功能：判断系统剪贴板中是否存在可粘贴文本。
static BOOL HasClipboardText()
{
	return (::IsClipboardFormatAvailable(CF_UNICODETEXT) ||
		::IsClipboardFormatAvailable(CF_TEXT));
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView

IMPLEMENT_DYNCREATE(CGoodYaView, CView)

BEGIN_MESSAGE_MAP(CGoodYaView, CView)
	//{{AFX_MSG_MAP(CGoodYaView)
	ON_COMMAND(IDM_COM, OnComDlg)
	ON_COMMAND(IDM_DEC, OnDecDlg)
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_PREVIEW_EDIT, OnPreviewEditChange)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditSelectAll)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 构造/析构

CGoodYaView::CGoodYaView()
{
	// 编辑框程序性回写时，避免触发递归同步。
	m_bSyncingEdit = FALSE;
}

CGoodYaView::~CGoodYaView()
{
}

// 功能：允许 MFC 在创建窗口前调整样式（当前保持默认）。
BOOL CGoodYaView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

// 功能：初始化视图时创建可编辑预览框并加载文档内容。
void CGoodYaView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	if (m_previewEdit.GetSafeHwnd() == NULL)
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		rcClient.DeflateRect(8, 8);

		// 开启横/纵滚动、自动换行控制和回车输入，支持大文本编辑。
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP |
			WS_VSCROLL | WS_HSCROLL |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
			ES_WANTRETURN | ES_NOHIDESEL;

		m_previewEdit.Create(dwStyle, rcClient, this, IDC_PREVIEW_EDIT);

		// 使用系统默认 GUI 字体，保证中英文显示稳定。
		CFont* pFont = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
		if (pFont != NULL)
			m_previewEdit.SetFont(pFont);
	}

	UpdateEditFromDocument();
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 绘制与更新

// 功能：绘制视图（文本由子编辑控件负责显示）。
void CGoodYaView::OnDraw(CDC* pDC)
{
	UNREFERENCED_PARAMETER(pDC);
}

// 功能：文档通知更新时，把最新文本同步到编辑框。
void CGoodYaView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	UNREFERENCED_PARAMETER(pSender);
	UNREFERENCED_PARAMETER(lHint);
	UNREFERENCED_PARAMETER(pHint);

	UpdateEditFromDocument();
}

// 功能：随视图大小变化调整编辑框区域。
void CGoodYaView::ResizePreviewEdit()
{
	if (m_previewEdit.GetSafeHwnd() == NULL)
		return;

	CRect rcClient;
	GetClientRect(&rcClient);
	rcClient.DeflateRect(8, 8);

	if (rcClient.Width() < 0)
		rcClient.right = rcClient.left;
	if (rcClient.Height() < 0)
		rcClient.bottom = rcClient.top;

	m_previewEdit.MoveWindow(&rcClient);
}

// 功能：把文档缓存（GBK 字节）刷新到编辑框（Unicode 显示）。
void CGoodYaView::UpdateEditFromDocument()
{
	if (m_previewEdit.GetSafeHwnd() == NULL)
		return;

	CGoodYaDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// 把编辑框当前内容也转 GBK 后比对，避免重复 SetWindowText 造成闪烁。
	CString editText = EditTextUnicodeToGbk(m_previewEdit.GetSafeHwnd());
	const CString& docText = pDoc->GetPreviewText();
	if (editText != docText)
	{
		m_bSyncingEdit = TRUE;
		SetEditTextFromGbk(m_previewEdit.GetSafeHwnd(), docText);
		m_previewEdit.SetSel(0, 0);
		m_previewEdit.SetModify(FALSE);
		m_bSyncingEdit = FALSE;
	}
}

// 功能：把编辑框最新文本同步回文档缓存（按 GBK 存储）。
void CGoodYaView::UpdateDocumentFromEdit()
{
	if (m_previewEdit.GetSafeHwnd() == NULL)
		return;

	CGoodYaDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CString text = EditTextUnicodeToGbk(m_previewEdit.GetSafeHwnd());
	pDoc->SetPreviewText(text, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 打印

// 功能：使用 MFC 默认打印准备流程。
BOOL CGoodYaView::OnPreparePrinting(CPrintInfo* pInfo)
{
	return DoPreparePrinting(pInfo);
}

// 功能：打印开始前预留扩展点。
void CGoodYaView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

// 功能：打印结束后预留清理扩展点。
void CGoodYaView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 调试辅助

#ifdef _DEBUG
// 功能：调试期对象有效性检查。
void CGoodYaView::AssertValid() const
{
	CView::AssertValid();
}

// 功能：调试期输出视图对象状态。
void CGoodYaView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CGoodYaDoc* CGoodYaView::GetDocument() // 非调试版本为内联实现
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGoodYaDoc)));
	return (CGoodYaDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 命令与消息

// 功能：弹出压缩参数对话框。
void CGoodYaView::OnComDlg()
{
	CComDlg comDlg;
	comDlg.DoModal();
}

// 功能：弹出解压参数对话框。
void CGoodYaView::OnDecDlg()
{
	CDecDlg decDlg;
	decDlg.DoModal();
}

// 功能：窗口尺寸变化时重排编辑框。
void CGoodYaView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	UNREFERENCED_PARAMETER(cx);
	UNREFERENCED_PARAMETER(cy);

	ResizePreviewEdit();
}

// 功能：监听编辑框文本变化并回写到文档。
void CGoodYaView::OnPreviewEditChange()
{
	// 程序回写期间忽略 EN_CHANGE，避免循环触发。
	if (m_bSyncingEdit)
		return;

	UpdateDocumentFromEdit();
}


// 功能：撤销编辑框最近一次可撤销的输入操作。
void CGoodYaView::OnEditUndo()
{
	HWND hWndEdit = m_previewEdit.GetSafeHwnd();
	if (hWndEdit == NULL || IsEditReadOnly(hWndEdit))
		return;

	if (GetFocus() != &m_previewEdit)
		m_previewEdit.SetFocus();

	if (m_previewEdit.CanUndo())
		m_previewEdit.Undo();
}

// 功能：剪切编辑框当前选中文本到剪贴板。
void CGoodYaView::OnEditCut()
{
	HWND hWndEdit = m_previewEdit.GetSafeHwnd();
	if (hWndEdit == NULL || IsEditReadOnly(hWndEdit))
		return;

	int nStart = 0;
	int nEnd = 0;
	m_previewEdit.GetSel(nStart, nEnd);
	if (nStart == nEnd)
		return;

	if (GetFocus() != &m_previewEdit)
		m_previewEdit.SetFocus();

	m_previewEdit.Cut();
}

// 功能：复制编辑框当前选中文本到剪贴板。
void CGoodYaView::OnEditCopy()
{
	if (m_previewEdit.GetSafeHwnd() == NULL)
		return;

	int nStart = 0;
	int nEnd = 0;
	m_previewEdit.GetSel(nStart, nEnd);
	if (nStart == nEnd)
		return;

	if (GetFocus() != &m_previewEdit)
		m_previewEdit.SetFocus();

	m_previewEdit.Copy();
}

// 功能：把剪贴板中的文本粘贴到编辑框当前光标位置。
void CGoodYaView::OnEditPaste()
{
	HWND hWndEdit = m_previewEdit.GetSafeHwnd();
	if (hWndEdit == NULL || IsEditReadOnly(hWndEdit))
		return;

	if (!HasClipboardText())
		return;

	if (GetFocus() != &m_previewEdit)
		m_previewEdit.SetFocus();

	m_previewEdit.Paste();
}

// 功能：选中编辑框中的全部文本。
void CGoodYaView::OnEditSelectAll()
{
	if (m_previewEdit.GetSafeHwnd() == NULL)
		return;

	if (GetFocus() != &m_previewEdit)
		m_previewEdit.SetFocus();

	m_previewEdit.SetSel(0, -1);
}

// 功能：仅当编辑框可撤销时启用“撤销”。
void CGoodYaView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	HWND hWndEdit = m_previewEdit.GetSafeHwnd();
	if (hWndEdit != NULL && !IsEditReadOnly(hWndEdit))
		bEnable = m_previewEdit.CanUndo();

	pCmdUI->Enable(bEnable);
}

// 功能：仅当存在选区且可编辑时启用“剪切”。
void CGoodYaView::OnUpdateEditCut(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	HWND hWndEdit = m_previewEdit.GetSafeHwnd();
	if (hWndEdit != NULL && !IsEditReadOnly(hWndEdit))
	{
		int nStart = 0;
		int nEnd = 0;
		m_previewEdit.GetSel(nStart, nEnd);
		bEnable = (nStart != nEnd);
	}

	pCmdUI->Enable(bEnable);
}

// 功能：仅当存在选区时启用“复制”。
void CGoodYaView::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	if (m_previewEdit.GetSafeHwnd() != NULL)
	{
		int nStart = 0;
		int nEnd = 0;
		m_previewEdit.GetSel(nStart, nEnd);
		bEnable = (nStart != nEnd);
	}

	pCmdUI->Enable(bEnable);
}

// 功能：仅当剪贴板有文本且编辑框可写时启用“粘贴”。
void CGoodYaView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	HWND hWndEdit = m_previewEdit.GetSafeHwnd();
	if (hWndEdit != NULL && !IsEditReadOnly(hWndEdit))
		bEnable = HasClipboardText();

	pCmdUI->Enable(bEnable);
}



// 功能：仅当编辑框存在文本时启用“全选”。
void CGoodYaView::OnUpdateEditSelectAll(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	HWND hWndEdit = m_previewEdit.GetSafeHwnd();
	if (hWndEdit != NULL)
		bEnable = (::GetWindowTextLength(hWndEdit) > 0);

	pCmdUI->Enable(bEnable);
}

