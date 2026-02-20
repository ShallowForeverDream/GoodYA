// GoodYaDoc.h : 文档类声明
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_)
#define AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGoodYaDoc : public CDocument
{
protected: // 仅供序列化创建
	CGoodYaDoc(); // 构造文档对象
	DECLARE_DYNCREATE(CGoodYaDoc)

public:
	const CString& GetPreviewText() const { return m_previewText; } // 获取当前编辑缓存文本
	void SetPreviewText(const CString& text, BOOL bMarkModified); // 更新缓存并按需标记已修改

	//{{AFX_VIRTUAL(CGoodYaDoc)
public:
	virtual BOOL OnNewDocument(); // 新建文档时清空缓存
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName); // 从文件读取文本到缓存
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName); // 把缓存文本保存到文件
	virtual void Serialize(CArchive& ar); // 预留 CArchive 序列化入口
	//}}AFX_VIRTUAL

public:
	virtual ~CGoodYaDoc(); // 析构文档对象
#ifdef _DEBUG
	virtual void AssertValid() const; // 调试有效性检查
	virtual void Dump(CDumpContext& dc) const; // 调试输出对象信息
#endif

protected:
	CString m_previewText; // 文档级文本缓存（按 GBK 字节存储）

protected:
	//{{AFX_MSG(CGoodYaDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 将在以前一行之前插入附加声明。

#endif // !defined(AFX_GOODYADOC_H__5937C9EC_0A08_4C59_965B_E8B838B6A1BE__INCLUDED_)