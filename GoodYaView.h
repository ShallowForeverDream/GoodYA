// GoodYaView.h : interface of the CGoodYaView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_)
#define AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CGoodYaView : public CView
{
protected: // create from serialization only
	CGoodYaView();
	DECLARE_DYNCREATE(CGoodYaView)

// Attributes
public:
	CGoodYaDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGoodYaView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGoodYaView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CGoodYaView)
	afx_msg void OnComDlg();
	afx_msg void OnDecDlg();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in GoodYaView.cpp
inline CGoodYaDoc* CGoodYaView::GetDocument()
   { return (CGoodYaDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_)
