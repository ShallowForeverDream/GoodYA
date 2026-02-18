// GoodYa.h : main header file for the GOODYA application
//

#if !defined(AFX_GOODYA_H__7DC62E83_0591_4D46_AE09_779A77815654__INCLUDED_)
#define AFX_GOODYA_H__7DC62E83_0591_4D46_AE09_779A77815654__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGoodYaApp:
// See GoodYa.cpp for the implementation of this class
//

class CGoodYaApp : public CWinApp
{
public:
	CGoodYaApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGoodYaApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CGoodYaApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOODYA_H__7DC62E83_0591_4D46_AE09_779A77815654__INCLUDED_)
