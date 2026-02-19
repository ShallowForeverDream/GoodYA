#if !defined(AFX_DECDLG_H__5791C5B5_76E2_481C_AAB2_231B0144C4D6__INCLUDED_)
#define AFX_DECDLG_H__5791C5B5_76E2_481C_AAB2_231B0144C4D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DecDlg.h : 解压设置对话框头文件
//

/////////////////////////////////////////////////////////////////////////////
// CDecDlg 对话框

class CDecDlg : public CDialog
{
// 构造
public:
	CDecDlg(CWnd* pParent = NULL);   // 标准构造函数

// 对话框数据
	//{{AFX_DATA(CDecDlg)
	enum { IDD = IDD_DIALOG2 };
	CTreeCtrl	m_treeCtrl;
	int		m_update;
	int		m_cover;
	//}}AFX_DATA

// 重写
	// ClassWizard 生成的虚函数重写
	//{{AFX_VIRTUAL(CDecDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 数据交换支持
	//}}AFX_VIRTUAL

// 实现
protected:
	BOOL GetInputPath(char outPath[500]);
	BOOL PromptPassword(CString& outPassword);

	// 消息映射函数
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
// Microsoft Visual C++ 会在上一行之前插入附加声明。

#endif // !defined(AFX_DECDLG_H__5791C5B5_76E2_481C_AAB2_231B0144C4D6__INCLUDED_)