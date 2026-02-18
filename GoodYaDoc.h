// GoodYaDoc.h : interface of the CGoodYaDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_)
#define AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CGoodYaDoc : public CDocument
{
protected: // create from serialization only
	CGoodYaDoc();
	DECLARE_DYNCREATE(CGoodYaDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGoodYaDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGoodYaDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CGoodYaDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_)
