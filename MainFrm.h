// MainFrm.h : 主框架窗口类声明
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__03C16912_4833_4D2A_95A9_B43560363211__INCLUDED_)
#define AFX_MAINFRM_H__03C16912_4833_4D2A_95A9_B43560363211__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
protected: // 仅供序列化创建
	CMainFrame(); // 构造主框架窗口
	DECLARE_DYNCREATE(CMainFrame)

public:
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs); // 创建前设置窗口样式
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle); // 刷新窗口标题文本
	//}}AFX_VIRTUAL

public:
	virtual ~CMainFrame(); // 析构主框架窗口
#ifdef _DEBUG
	virtual void AssertValid() const; // 调试有效性检查
	virtual void Dump(CDumpContext& dc) const; // 调试输出对象信息
#endif

protected:  // 内嵌控件栏
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct); // 创建工具栏和状态栏
	afx_msg void OnTimer(UINT nIDEvent); // 定时刷新状态栏时间
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized); // 窗口激活状态变化
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__03C16912_4833_4D2A_95A9_B43560363211__INCLUDED_)
