// GoodYaDoc.cpp : 文档类实现
//

#include "stdafx.h"
#include "GoodYa.h"

#include "GoodYaDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc

IMPLEMENT_DYNCREATE(CGoodYaDoc, CDocument)

BEGIN_MESSAGE_MAP(CGoodYaDoc, CDocument)
	//{{AFX_MSG_MAP(CGoodYaDoc)
		// ClassWizard 生成区域
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc 构造/析构

CGoodYaDoc::CGoodYaDoc()
{
	// 构造阶段不需要额外初始化。
}

CGoodYaDoc::~CGoodYaDoc()
{
}

// 功能：更新文档缓存文本，并按需标记“已修改”。
void CGoodYaDoc::SetPreviewText(const CString& text, BOOL bMarkModified)
{
	// 当文本未变化时直接返回，避免无意义刷新。
	if (m_previewText == text)
		return;

	m_previewText = text;
	if (bMarkModified)
		SetModifiedFlag(TRUE);
}

// 功能：新建文档时清空编辑区缓存。
BOOL CGoodYaDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_previewText.Empty();
	return TRUE;
}

// 功能：按 GBK 字节读取文本文件，并同步到视图编辑区。
BOOL CGoodYaDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	// 按二进制方式读取，避免文本模式改写换行或编码。
	CFile file;
	if (!file.Open(lpszPathName, CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary))
	{
		AfxMessageBox(_T("打开文件失败！"));
		return FALSE;
	}

	DWORD dwLen = (DWORD)file.GetLength();
	CString text;
	if (dwLen > 0)
	{
		char* pBuf = new char[dwLen + 1];
		if (pBuf == NULL)
		{
			file.Close();
			AfxMessageBox(_T("内存不足，读取失败！"));
			return FALSE;
		}

		UINT nRead = file.Read(pBuf, dwLen);
		pBuf[nRead] = 0;
		text = pBuf;
		delete[] pBuf;
	}
	file.Close();

	// 刷新文档缓存并通知视图更新。
	SetPreviewText(text, FALSE);
	UpdateAllViews(NULL);
	SetModifiedFlag(FALSE);
	return TRUE;
}

// 功能：把当前编辑内容按 GBK 字节保存到文件。
BOOL CGoodYaDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	CFile file;
	if (!file.Open(lpszPathName, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeBinary))
	{
		AfxMessageBox(_T("保存文件失败！"));
		return FALSE;
	}

	// 直接写入缓存字节，保持 GBK 内容不被运行时再次转码。
	int nLen = m_previewText.GetLength();
	if (nLen > 0)
		file.Write((LPCSTR)m_previewText, nLen);
	file.Close();

	SetModifiedFlag(FALSE);
	return TRUE;
}

// 功能：预留序列化入口（当前项目主要走 OnOpen/OnSave）。
void CGoodYaDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// 可在此扩展 CArchive 存储逻辑。
	}
	else
	{
		// 可在此扩展 CArchive 读取逻辑。
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc 调试辅助

#ifdef _DEBUG
// 功能：调试期对象有效性检查。
void CGoodYaDoc::AssertValid() const
{
	CDocument::AssertValid();
}

// 功能：调试期输出文档对象状态。
void CGoodYaDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc 命令