#if !defined(AFX_PASSDLG_H__F4A1A6B1_5A77_4F90_B5C2_AA8A6EAF5218__INCLUDED_)
#define AFX_PASSDLG_H__F4A1A6B1_5A77_4F90_B5C2_AA8A6EAF5218__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// PassDlg.h : 密码输入对话框头文件

class CPassDlg : public CDialog
{
public:
	CPassDlg(LPCTSTR lpszTitle, LPCTSTR lpszPrompt, CWnd* pParent = NULL);

	//{{AFX_DATA(CPassDlg)
	enum { IDD = IDD_PASSDLG };
	CString	m_password;
	//}}AFX_DATA

protected:
	CString m_title;
	CString m_prompt;

	//{{AFX_VIRTUAL(CPassDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_PASSDLG_H__F4A1A6B1_5A77_4F90_B5C2_AA8A6EAF5218__INCLUDED_)