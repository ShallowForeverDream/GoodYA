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
	//{{AFX_VIRTUAL(CDecDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // 绑定控件与成员变量
	//}}AFX_VIRTUAL

// 实现
protected:
	BOOL GetInputPath(char outPath[500]); // 读取输入框中的 .huf/.zjh 文件路径
	BOOL PromptPassword(CString& outPassword); // 弹框获取解压密码

	// 注意：不再使用图标，树控件只显示文本层次结构

	// 消息映射函数
	//{{AFX_MSG(CDecDlg)
	afx_msg void OnDecDlgCancel(); // 点击“取消”后关闭对话框
	afx_msg void OnDecDlgOk(); // 校验输入并执行解压流程
	afx_msg void OnBrowse(); // 浏览并选择压缩包文件
	afx_msg void InitTree(); // 初始化目录树根节点
	afx_msg void PopulateTree(HTREEITEM hParent, LPCTSTR path); // 递归填充目录树
	virtual BOOL OnInitDialog(); // 初始化对话框显示状态
	afx_msg void OnCancelMode(); // 取消模式消息，保持默认处理
	// 树控件相关消息处理：
	afx_msg void OnTvnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult); // 当树节点将展开时填充子节点
	afx_msg void OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult); // 当选择变化时把完整路径回填到目标路径编辑框
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 将在以前一行之前插入附加声明。

#endif // !defined(AFX_DECDLG_H__5791C5B5_76E2_481C_AAB2_231B0144C4D6__INCLUDED_)
