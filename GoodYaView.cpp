// GoodYaView.cpp : implementation of the CGoodYaView class
//

#include "stdafx.h"
#include "GoodYa.h"

#include "GoodYaDoc.h"
#include "GoodYaView.h"
#include "ComDlg.h"
#include "DecDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView

IMPLEMENT_DYNCREATE(CGoodYaView, CView)

BEGIN_MESSAGE_MAP(CGoodYaView, CView)
	//{{AFX_MSG_MAP(CGoodYaView)
	ON_COMMAND(IDM_COM, OnComDlg)
	ON_COMMAND(IDM_DEC, OnDecDlg)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView construction/destruction

CGoodYaView::CGoodYaView()
{
	// TODO: add construction code here

}

CGoodYaView::~CGoodYaView()
{
}

BOOL CGoodYaView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView drawing

void CGoodYaView::OnDraw(CDC* pDC)
{
	CGoodYaDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView printing

BOOL CGoodYaView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CGoodYaView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CGoodYaView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView diagnostics

#ifdef _DEBUG
void CGoodYaView::AssertValid() const
{
	CView::AssertValid();
}

void CGoodYaView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CGoodYaDoc* CGoodYaView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGoodYaDoc)));
	return (CGoodYaDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGoodYaView message handlers

void CGoodYaView::OnComDlg() 
{
	// TODO: Add your command handler code here
	//MessageBox("ComBtn clicked!");
	CComDlg comDlg;
	comDlg.DoModal();	
}

void CGoodYaView::OnDecDlg() 
{
	// TODO: Add your command handler code here
	//MessageBox("DecBtn clicked!");
	CDecDlg decDlg;
	decDlg.DoModal();	
}
