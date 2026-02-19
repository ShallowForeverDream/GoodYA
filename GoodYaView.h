// GoodYaView.h : 视图类接口声明
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_)
#define AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CGoodYaView : public CView
{
protected: // 仅用于序列化创建
	CGoodYaView();
	DECLARE_DYNCREATE(CGoodYaView)

// 属性
public:
	CGoodYaDoc* GetDocument();

// 操作
public:

// 重写
	// ClassWizard 生成的虚函数重写
	//{{AFX_VIRTUAL(CGoodYaView)
	public:
	virtual void OnDraw(CDC* pDC);  // 重写：绘制视图内容
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// 实现
public:
	virtual ~CGoodYaView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CEdit m_previewEdit;
	BOOL m_bSyncingEdit;
	void ResizePreviewEdit();
	void UpdateEditFromDocument();
	void UpdateDocumentFromEdit();

// 消息映射函数
protected:
	//{{AFX_MSG(CGoodYaView)
	afx_msg void OnComDlg();
	afx_msg void OnDecDlg();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPreviewEditChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // 调试版本在 GoodYaView.cpp 中实现
inline CGoodYaDoc* CGoodYaView::GetDocument()
   { return (CGoodYaDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 会在上一行之前插入附加声明。

#endif // !defined(AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_)