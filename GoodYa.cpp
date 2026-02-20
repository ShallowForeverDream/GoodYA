// GoodYa.cpp : 应用程序主入口与框架初始化
//

#include "stdafx.h"
#include "GoodYa.h"

#include "MainFrm.h"
#include "GoodYaDoc.h"
#include "GoodYaView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoodYaApp

BEGIN_MESSAGE_MAP(CGoodYaApp, CWinApp)
	//{{AFX_MSG_MAP(CGoodYaApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - ClassWizard 自动维护区域
	//}}AFX_MSG_MAP
	// 标准文档命令
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// 打印设置命令
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoodYaApp 构造

CGoodYaApp::CGoodYaApp()
{
	// 主要初始化逻辑放在 InitInstance。
}

/////////////////////////////////////////////////////////////////////////////
// 全局应用对象

CGoodYaApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CGoodYaApp 初始化

/**
 * 功能：初始化 MFC 应用、文档模板与主窗口。
 * 设置控制容器、注册文档模板并显示主窗口。
 * @return BOOL
 * @author zxl
 * @date 2024-06-01
 */
BOOL CGoodYaApp::InitInstance()
{
	AfxEnableControlContainer();

#ifdef _AFXDLL
	Enable3dControls();
#else
	Enable3dControlsStatic();
#endif

	// 设置应用配置注册表路径。
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	// 加载标准配置（含最近文件列表）。
	LoadStdProfileSettings();

	// 注册 SDI 文档模板，关联 文档/主框架/视图 三类对象。
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CGoodYaDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(CGoodYaView));
	AddDocTemplate(pDocTemplate);

	// 解析并执行命令行（支持外部打开文件）。
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// 显示并刷新主窗口。
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// 关于对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

/**
 * 功能：执行关于对话框控件的数据交换。
 * @param CDataExchange* pDX
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/**
 * 功能：显示“关于”对话框。
 * 弹出关于对话框并阻塞直到关闭。
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
void CGoodYaApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
