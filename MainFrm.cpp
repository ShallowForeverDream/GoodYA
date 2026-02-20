// MainFrm.cpp : 主框架窗口实现
//

#include "stdafx.h"
#include "GoodYa.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_ACTIVATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,
	IDS_TIMER,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// 功能：设置主窗口标题“好压V1.0”，并兼容 ANSI/Unicode 窗口 API。
static void ApplyMainTitle(HWND hWnd)
{
	const WCHAR kTitleW[] = { 0x597D, 0x538B, L'V', L'1', L'.', L'0', 0 }; // 好压V1.0
	char titleA[64] = {0};
	int n = WideCharToMultiByte(GetACP(), 0, kTitleW, -1, titleA, sizeof(titleA), NULL, NULL);
	if (n > 0)
		::SetWindowTextA(hWnd, titleA);
	else
		::SetWindowTextW(hWnd, kTitleW);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame 构造/析构

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

// 功能：创建工具栏、状态栏并完成主窗口初始化。
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 启动 1 秒刷新一次的时钟定时器。
	SetTimer(1, 1000, NULL);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;
	}

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	ApplyMainTitle(m_hWnd);
	return 0;
}

// 功能：创建窗口前设置框架样式与默认标题。
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWnd::PreCreateWindow(cs))
		return FALSE;

	cs.style = WS_OVERLAPPEDWINDOW;
	cs.lpszName = "GoodYaV1.0";

	return TRUE;
}

// 功能：更新框架标题时始终显示自定义主标题。
void CMainFrame::OnUpdateFrameTitle(BOOL /*bAddToTitle*/)
{
	ApplyMainTitle(m_hWnd);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame 调试辅助

#ifdef _DEBUG
// 功能：调试期对象有效性检查。
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

// 功能：调试期输出框架窗口状态。
void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame 消息处理

// 功能：每秒刷新一次状态栏时钟文本。
void CMainFrame::OnTimer(UINT nIDEvent)
{
	CTime t = CTime::GetCurrentTime();
	CString str = t.Format("%H:%M:%S");

	int index = m_wndStatusBar.CommandToIndex(IDS_TIMER);

	// 按当前字符串宽度动态调整状态栏面板宽度，避免时间显示被截断。
	CClientDC dc(this);
	CSize sz = dc.GetTextExtent(str);
	m_wndStatusBar.SetPaneInfo(index, IDS_TIMER, SBPS_NORMAL, sz.cx);
	m_wndStatusBar.SetPaneText(index, str);

	CFrameWnd::OnTimer(nIDEvent);
}

// 功能：窗口激活状态变化时保持默认处理。
void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
}