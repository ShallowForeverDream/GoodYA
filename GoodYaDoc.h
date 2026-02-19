// GoodYaDoc.h : 文档类接口声明
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_)
#define AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CGoodYaDoc : public CDocument
{
protected: // 仅用于序列化创建
	CGoodYaDoc();
	DECLARE_DYNCREATE(CGoodYaDoc)

// 属性
public:

// 操作
public:
	const CString& GetPreviewText() const { return m_previewText; }
	void SetPreviewText(const CString& text, BOOL bMarkModified);

// 重写
	// ClassWizard 生成的虚函数重写
	//{{AFX_VIRTUAL(CGoodYaDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// 实现
public:
	virtual ~CGoodYaDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CString m_previewText;

// 消息映射函数
protected:
	//{{AFX_MSG(CGoodYaDoc)
		// 说明：ClassWizard 会在此添加或移除成员函数。
		//    请勿手工修改这些由 ClassWizard 生成的代码块。
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 会在上一行之前插入附加声明。

#endif // !defined(AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_)