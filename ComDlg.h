#if !defined(AFX_COMDLG_H__1010E85E_4FBD_4BB3_A793_6CF0CBF7683C__INCLUDED_)
#define AFX_COMDLG_H__1010E85E_4FBD_4BB3_A793_6CF0CBF7683C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ComDlg.h : 压缩设置对话框头文件
//

/////////////////////////////////////////////////////////////////////////////
// CComDlg 对话框

class CComDlg : public CDialog
{
// 构造
public:
	CComDlg(CWnd* pParent = NULL);   // 标准构造函数

// 对话框数据
	//{{AFX_DATA(CComDlg)
	enum { IDD = IDD_DIALOG1 };
	int		m_Extend;
	BOOL	m_isDelete;
	BOOL	m_isCheck;
	BOOL	m_isLock;
	//}}AFX_DATA

	CString m_password; // 压缩密码（明文仅保存在本次对话框生命周期）

// 重写
	// ClassWizard 生成的虚函数重写
	//{{AFX_VIRTUAL(CComDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 数据交换支持
	//}}AFX_VIRTUAL

// 实现
protected:
	BOOL GetInputPath(char outPath[500]);
	BOOL GetPasswordA(char outPass[129]);

	// 消息映射函数
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
// Microsoft Visual C++ 会在上一行之前插入附加声明。

#endif // !defined(AFX_COMDLG_H__1010E85E_4FBD_4BB3_A793_6CF0CBF7683C__INCLUDED_)