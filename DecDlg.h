#if !defined(AFX_DECDLG_H__5791C5B5_76E2_481C_AAB2_231B0144C4D6__INCLUDED_)
#define AFX_DECDLG_H__5791C5B5_76E2_481C_AAB2_231B0144C4D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DecDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDecDlg dialog

class CDecDlg : public CDialog
{
// Construction
public:
	CDecDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDecDlg)
	enum { IDD = IDD_DIALOG2 };
	CTreeCtrl	m_treeCtrl;
	int		m_update;
	int		m_cover;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDecDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDecDlg)
	afx_msg void OnDecDlgCancel();
	afx_msg void OnDecDlgOk();
	afx_msg void OnBrowse();
	afx_msg void InitTree();
	afx_msg void PopulateTree(HTREEITEM hParent, LPCTSTR path);
	virtual BOOL OnInitDialog();
	afx_msg void OnCancelMode();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DECDLG_H__5791C5B5_76E2_481C_AAB2_231B0144C4D6__INCLUDED_)
