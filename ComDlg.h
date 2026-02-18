#if !defined(AFX_COMDLG_H__1010E85E_4FBD_4BB3_A793_6CF0CBF7683C__INCLUDED_)
#define AFX_COMDLG_H__1010E85E_4FBD_4BB3_A793_6CF0CBF7683C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ComDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CComDlg dialog

class CComDlg : public CDialog
{
// Construction
public:
	CComDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CComDlg)
	enum { IDD = IDD_DIALOG1 };
	int		m_Extend;
	BOOL	m_isDelete;
	BOOL	m_isCheck;
	BOOL	m_isLock;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CComDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CComDlg)
	afx_msg void OnCancel();
	afx_msg void OnOk();
	virtual BOOL OnInitDialog();
	afx_msg void OnCancelMode();
	afx_msg void OnSetpass();
	afx_msg void OnOpen();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMDLG_H__1010E85E_4FBD_4BB3_A793_6CF0CBF7683C__INCLUDED_)
