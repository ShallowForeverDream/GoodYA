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

// 预览编辑框控件 ID
#define IDC_PREVIEW_EDIT 50001

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView

IMPLEMENT_DYNCREATE(CGoodYaView, CView)

BEGIN_MESSAGE_MAP(CGoodYaView, CView)
	//{{AFX_MSG_MAP(CGoodYaView)
	ON_COMMAND(IDM_COM, OnComDlg)
	ON_COMMAND(IDM_DEC, OnDecDlg)
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_PREVIEW_EDIT, OnPreviewEditChange)
	//}}AFX_MSG_MAP
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 构造/析构

CGoodYaView::CGoodYaView()
{
	m_bSyncingEdit = FALSE;
}

CGoodYaView::~CGoodYaView()
{
}

BOOL CGoodYaView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

void CGoodYaView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	if (m_previewEdit.GetSafeHwnd() == NULL)
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		rcClient.DeflateRect(8, 8);

		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP |
			WS_VSCROLL | WS_HSCROLL |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
			ES_WANTRETURN | ES_NOHIDESEL;

		m_previewEdit.Create(dwStyle, rcClient, this, IDC_PREVIEW_EDIT);

		// 使用系统界面字体，保证中文显示稳定
		CFont* pFont = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
		if (pFont != NULL)
			m_previewEdit.SetFont(pFont);
	}

	UpdateEditFromDocument();
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 绘制

void CGoodYaView::OnDraw(CDC* pDC)
{
	UNREFERENCED_PARAMETER(pDC);
	// 文本由子编辑控件自行绘制
}

void CGoodYaView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	UNREFERENCED_PARAMETER(pSender);
	UNREFERENCED_PARAMETER(lHint);
	UNREFERENCED_PARAMETER(pHint);

	UpdateEditFromDocument();
}

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

void CGoodYaView::UpdateEditFromDocument()
{
	if (m_previewEdit.GetSafeHwnd() == NULL)
		return;

	CGoodYaDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CString editText;
	m_previewEdit.GetWindowText(editText);

	const CString& docText = pDoc->GetPreviewText();
	if (editText != docText)
	{
		m_bSyncingEdit = TRUE;
		m_previewEdit.SetWindowText(docText);
		m_previewEdit.SetSel(0, 0);
		m_previewEdit.SetModify(FALSE);
		m_bSyncingEdit = FALSE;
	}
}

void CGoodYaView::UpdateDocumentFromEdit()
{
	if (m_previewEdit.GetSafeHwnd() == NULL)
		return;

	CGoodYaDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CString text;
	m_previewEdit.GetWindowText(text);
	pDoc->SetPreviewText(text, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 打印

BOOL CGoodYaView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 使用 MFC 默认打印准备流程
	return DoPreparePrinting(pInfo);
}

void CGoodYaView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// 打印前预留扩展点
}

void CGoodYaView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// 打印后预留清理点
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 诊断

#ifdef _DEBUG
void CGoodYaView::AssertValid() const
{
	CView::AssertValid();
}

void CGoodYaView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CGoodYaDoc* CGoodYaView::GetDocument() // 非调试版本内联实现
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGoodYaDoc)));
	return (CGoodYaDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView 消息处理

void CGoodYaView::OnComDlg() 
{
	CComDlg comDlg;
	comDlg.DoModal();
}

void CGoodYaView::OnDecDlg() 
{
	CDecDlg decDlg;
	decDlg.DoModal();
}

void CGoodYaView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	UNREFERENCED_PARAMETER(cx);
	UNREFERENCED_PARAMETER(cy);

	ResizePreviewEdit();
}

void CGoodYaView::OnPreviewEditChange()
{
	if (m_bSyncingEdit)
		return;

	// 用户编辑后同步回文档，支持文件保存
	UpdateDocumentFromEdit();
}