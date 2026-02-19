// PassDlg.cpp : 密码输入对话框实现
//

#include "stdafx.h"
#include "GoodYa.h"
#include "PassDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPassDlg::CPassDlg(LPCTSTR lpszTitle, LPCTSTR lpszPrompt, CWnd* pParent)
	: CDialog(CPassDlg::IDD, pParent)
{
	m_title = (lpszTitle == NULL) ? _T("输入密码") : lpszTitle;
	m_prompt = (lpszPrompt == NULL) ? _T("请输入密码：") : lpszPrompt;
	m_password = _T("");
}

void CPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPassDlg)
	DDX_Text(pDX, IDC_EDIT_PASS, m_password);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPassDlg, CDialog)
	//{{AFX_MSG_MAP(CPassDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CPassDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置标题和提示语，复用于“设置密码”和“输入解压密码”
	SetWindowText(m_title);
	SetDlgItemText(IDC_STATIC_PASS_PROMPT, m_prompt);
	SetDlgItemText(IDC_EDIT_PASS, m_password);

	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_PASS);
	if (pEdit != NULL)
	{
		pEdit->SetLimitText(128);
		pEdit->SetFocus();
		pEdit->SetSel(0, -1);
	}

	return FALSE;
}