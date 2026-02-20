// GoodYa.h : GOODYA 应用程序主头文件
//

#if !defined(AFX_GOODYA_H__7DC62E83_0591_4D46_AE09_779A77815654__INCLUDED_)
#define AFX_GOODYA_H__7DC62E83_0591_4D46_AE09_779A77815654__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // 主资源符号

/////////////////////////////////////////////////////////////////////////////
// CGoodYaApp

class CGoodYaApp : public CWinApp
{
public:
	CGoodYaApp(); // 构造应用对象

	//{{AFX_VIRTUAL(CGoodYaApp)
public:
	virtual BOOL InitInstance(); // 初始化应用、文档模板和主窗口
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CGoodYaApp)
	afx_msg void OnAppAbout(); // 响应“关于”菜单并弹出关于对话框
		// ClassWizard 自动维护区域
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOODYA_H__7DC62E83_0591_4D46_AE09_779A77815654__INCLUDED_)