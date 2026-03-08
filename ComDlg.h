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

	CString m_password; // 当前压缩密码（仅在本次对话生命周期内使用）
	CProgressCtrl m_zjhProgress; // ZJH 压缩进度条（运行时动态创建）
	CStatic m_zjhProgressText;   // ZJH 进度说明文本（显示阶段、耗时、ETA）
	BOOL m_progressUiCreated;    // 是否已完成动态控件创建

// 重写
	//{{AFX_VIRTUAL(CComDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // 绑定控件与成员变量
	//}}AFX_VIRTUAL

// 实现
protected:
	BOOL GetInputPath(char outPath[500]); // 读取输入框中的待压缩文件路径
	BOOL GetPasswordA(char outPass[129]); // 将密码转换为 ANSI 字节串供底层算法使用
	void EnsureProgressUi(); // 创建并初始化进度条控件
	void ResetProgressUi(); // 重置进度显示文本和进度值
	void SetCompressionUiEnabled(BOOL enabled); // 压缩期间禁用交互控件，避免重复触发
	void UpdateProgressUi(int percent, int stage, unsigned long elapsedMs, unsigned long etaMs); // 刷新进度显示
	CString FormatDurationText(unsigned long milliseconds) const; // 毫秒转中文可读时长
	static void OnZjhProgress(int percent, int stage, unsigned long elapsedMs, unsigned long etaMs, void* userData); // zjh_codec 回调入口

	// 消息映射函数
	//{{AFX_MSG(CComDlg)
	afx_msg void OnCancel(); // 点击“取消”后关闭对话框
	afx_msg void OnOk(); // 校验输入并执行压缩流程
	virtual BOOL OnInitDialog(); // 初始化下拉选项和默认值
	afx_msg void OnCancelMode(); // 取消模式消息，保持默认处理
	afx_msg void OnSetpass(); // 打开密码输入框并更新加密状态
	afx_msg void OnOpen(); // 浏览并选择要压缩的源文件
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 将在以前一行之前插入附加声明。

#endif // !defined(AFX_COMDLG_H__1010E85E_4FBD_4BB3_A793_6CF0CBF7683C__INCLUDED_)
