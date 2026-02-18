// GoodYaDoc.cpp : implementation of the CGoodYaDoc class
//

#include "stdafx.h"
#include "GoodYa.h"

#include "GoodYaDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc

IMPLEMENT_DYNCREATE(CGoodYaDoc, CDocument)

BEGIN_MESSAGE_MAP(CGoodYaDoc, CDocument)
	//{{AFX_MSG_MAP(CGoodYaDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc construction/destruction

CGoodYaDoc::CGoodYaDoc()
{
	// TODO: add one-time construction code here

}

CGoodYaDoc::~CGoodYaDoc()
{
}

BOOL CGoodYaDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc serialization

void CGoodYaDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc diagnostics

#ifdef _DEBUG
void CGoodYaDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CGoodYaDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGoodYaDoc commands
