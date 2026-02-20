// GoodYaView.h : 视图类声明
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_)
#define AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGoodYaView : public CView
{
protected: // 仅供序列化创建
	CGoodYaView(); // 构造视图对象
	DECLARE_DYNCREATE(CGoodYaView)

public:
	CGoodYaDoc* GetDocument(); // 获取关联文档对象

	//{{AFX_VIRTUAL(CGoodYaView)
public:
	virtual void OnDraw(CDC* pDC);  // 视图绘制入口（本项目主要由编辑控件显示）
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs); // 创建窗口前样式钩子
	virtual void OnInitialUpdate(); // 视图首次更新时创建编辑控件并加载文本
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint); // 文档更新时同步编辑区
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo); // 打印准备
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo); // 打印开始
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo); // 打印结束
	//}}AFX_VIRTUAL

public:
	virtual ~CGoodYaView(); // 析构视图对象
#ifdef _DEBUG
	virtual void AssertValid() const; // 调试有效性检查
	virtual void Dump(CDumpContext& dc) const; // 调试输出对象信息
#endif

protected:
	CEdit m_previewEdit; // 可编辑文本预览控件
	BOOL m_bSyncingEdit; // 防止文档->编辑框回写触发递归更新
	void ResizePreviewEdit(); // 根据窗口大小重排编辑框
	void UpdateEditFromDocument(); // 文档缓存同步到编辑框显示
	void UpdateDocumentFromEdit(); // 编辑内容同步回文档缓存

protected:
	//{{AFX_MSG(CGoodYaView)
	afx_msg void OnComDlg(); // 打开压缩设置对话框
	afx_msg void OnDecDlg(); // 打开解压设置对话框
	afx_msg void OnSize(UINT nType, int cx, int cy); // 处理窗口尺寸变化
	afx_msg void OnPreviewEditChange(); // 响应编辑框文本变化
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // 非调试版本在 GoodYaView.cpp 中实现
inline CGoodYaDoc* CGoodYaView::GetDocument()
   { return (CGoodYaDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ 将在以前一行之前插入附加声明。

#endif // !defined(AFX_GOODYAVIEW_H__30F1EF70_D7D9_484D_A876_F637169A22D4__INCLUDED_)