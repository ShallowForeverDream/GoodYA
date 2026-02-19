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
		// 说明：ClassWizard 会在此添加或移除消息映射宏。
		//    请勿手工修改这些由 ClassWizard 生成的代码块。
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc 构造/析构

CGoodYaDoc::CGoodYaDoc()
{
	// 构造阶段暂不需要额外初始化
}

CGoodYaDoc::~CGoodYaDoc()
{
}

void CGoodYaDoc::SetPreviewText(const CString& text, BOOL bMarkModified)
{
	if (m_previewText == text)
		return;

	m_previewText = text;
	if (bMarkModified)
		SetModifiedFlag(TRUE);
}

BOOL CGoodYaDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// 新建文档时清空编辑内容
	m_previewText.Empty();
	return TRUE;
}

BOOL CGoodYaDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	// 先走基类流程，确保文档状态由框架正确初始化
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	CStdioFile file;
	if (!file.Open(lpszPathName, CFile::modeRead | CFile::shareDenyWrite | CFile::typeText))
	{
		AfxMessageBox(_T("打开文件失败！"));
		return FALSE;
	}

	CString text;
	CString line;
	BOOL bFirstLine = TRUE;

	// 读取完整文本，支持后续编辑和保存
	while (file.ReadString(line))
	{
		if (!bFirstLine)
			text += _T("\r\n");
		text += line;
		bFirstLine = FALSE;
	}
	file.Close();

	SetPreviewText(text, FALSE);
	UpdateAllViews(NULL);
	SetModifiedFlag(FALSE);
	return TRUE;
}

BOOL CGoodYaDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	CStdioFile file;
	if (!file.Open(lpszPathName, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText))
	{
		AfxMessageBox(_T("保存文件失败！"));
		return FALSE;
	}

	// 按文本方式保存当前编辑内容
	file.WriteString(m_previewText);
	file.Close();

	SetModifiedFlag(FALSE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc 序列化

void CGoodYaDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// 如需二进制归档，可在此补充存储逻辑
	}
	else
	{
		// 如需二进制归档，可在此补充加载逻辑
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc 诊断

#ifdef _DEBUG
void CGoodYaDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CGoodYaDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc 命令