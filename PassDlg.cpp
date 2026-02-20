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

// 功能：构造密码对话框，支持外部传入标题与提示语。
CPassDlg::CPassDlg(LPCTSTR lpszTitle, LPCTSTR lpszPrompt, CWnd* pParent)
	: CDialog(CPassDlg::IDD, pParent)
{
	m_title = (lpszTitle == NULL) ? _T("输入密码") : lpszTitle;
	m_prompt = (lpszPrompt == NULL) ? _T("请输入密码：") : lpszPrompt;
	m_password = _T("");
}

// 功能：绑定密码输入框和成员变量。
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

// 功能：初始化密码对话框标题、提示文本与输入焦点。
BOOL CPassDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置对话框文案，区分“压缩密码”和“解压密码”两种场景。
	SetWindowText(m_title);
	SetDlgItemText(IDC_STATIC_PASS_PROMPT, m_prompt);
	SetDlgItemText(IDC_EDIT_PASS, m_password);

	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_PASS);
	if (pEdit != NULL)
	{
		// 限制密码长度并自动聚焦，方便用户直接输入。
		pEdit->SetLimitText(128);
		pEdit->SetFocus();
		pEdit->SetSel(0, -1);
	}

	// 返回 FALSE，表示焦点已手动设置到密码输入框。
	return FALSE;
}