// DecDlg.cpp : 解压设置对话框实现
//

#include "stdafx.h"
#include "GoodYa.h"
#include "DecDlg.h"
#include "Huffman.h"
#include "PassDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// 功能：ToDialogTextByACP，把 Unicode 文本转换为当前进程使用的 TCHAR 文本。
static CString ToDialogTextByACP(const wchar_t* textW)
{
	// 代码段功能：在非 Unicode 编译下按 ACP 编码转换，避免中文乱码。
	if (textW == NULL)
		return _T("");
#ifdef _UNICODE
	return CString(textW);
#else
	char textA[1024] = {0};
	if (WideCharToMultiByte(CP_ACP, 0, textW, -1, textA, sizeof(textA), NULL, NULL) <= 0)
		return _T("");
	return CString(textA);
#endif
}

// 功能：ShowMsgByACP，统一消息框文本显示，避免直接弹框时中文乱码。
static void ShowMsgByACP(const wchar_t* textW)
{
	// 代码段功能：通过统一转换后调用 AfxMessageBox。
	AfxMessageBox(ToDialogTextByACP(textW));
}

/////////////////////////////////////////////////////////////////////////////
// CDecDlg 对话框

CDecDlg::CDecDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDecDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDecDlg)
	m_update = 0;
	m_cover = 0;
	//}}AFX_DATA_INIT
}

// 功能：绑定解压设置对话框控件和成员变量。
void CDecDlg::DoDataExchange(CDataExchange* pDX)
{
	// 代码段功能：执行 DDX/DDV 数据交换。
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDecDlg)
	DDX_Control(pDX, IDC_TREE1, m_treeCtrl);
	DDX_Radio(pDX, IDC_RADIO1, m_update);
	DDX_Radio(pDX, IDC_RADIO4, m_cover);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDecDlg, CDialog)
	//{{AFX_MSG_MAP(CDecDlg)
	ON_BN_CLICKED(IDC_BUTTON2, OnDecDlgCancel)
	ON_BN_CLICKED(IDC_BUTTON1, OnDecDlgOk)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_WM_CANCELMODE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDecDlg 消息处理

// 功能：处理“取消”按钮，关闭解压设置对话框。
void CDecDlg::OnDecDlgCancel()
{
	// 代码段功能：返回 IDCANCEL 给调用方。
	EndDialog(IDCANCEL);
}

// 读取输入路径并转换为 ANSI，便于当前 Huffman 接口使用。
// 功能：获取待解压文件路径。
BOOL CDecDlg::GetInputPath(char outPath[500])
{
	// 代码段功能：从编辑框读取路径并做编码转换与校验。
	if (outPath == NULL)
		return FALSE;

	TCHAR savePathText[500] = {0};
	GetDlgItem(IDC_EDIT2)->GetWindowText(savePathText, 500);

#ifdef _UNICODE
	if (WideCharToMultiByte(CP_ACP, 0, savePathText, -1, outPath, 500, NULL, NULL) == 0)
	{
		ShowMsgByACP(L"路径转换失败！");
		return FALSE;
	}
#else
	strncpy(outPath, savePathText, 499);
	outPath[499] = '\0';
#endif

	if (outPath[0] == '\0')
	{
		ShowMsgByACP(L"请选择 .huf 文件！");
		return FALSE;
	}
	return TRUE;
}

// 弹出密码输入框，供解压时验证密码。
// 功能：获取解压密码。
BOOL CDecDlg::PromptPassword(CString& outPassword)
{
	// 代码段功能：弹出密码输入框并校验非空。
	CPassDlg dlg(
		ToDialogTextByACP(L"输入解压密码"),
		ToDialogTextByACP(L"解压文件已加密，请输入密码："),
		this);
	if (dlg.DoModal() != IDOK)
		return FALSE;

	outPassword = dlg.m_password;
	if (outPassword.IsEmpty())
	{
		ShowMsgByACP(L"密码不能为空！");
		return FALSE;
	}
	return TRUE;
}

// 读取压缩包并解压，支持密码校验和 CRC 校验。
// 功能：执行解压流程。
void CDecDlg::OnDecDlgOk()
{
	// 代码段功能：执行解压主流程。
	char savePath[500] = {0};
	if (!GetInputPath(savePath))
		return;

	Huffman huffman;
	BOOL hasPassword = FALSE;
	unsigned long storedCrc = 0;
	char md5Hex[33] = {0};
	if (!huffman.GetPackageInfo(savePath, &hasPassword, &storedCrc, md5Hex))
	{
		ShowMsgByACP(L"读取压缩文件信息失败！");
		return;
	}

	CString password;
	char passA[129] = {0};
	const char* pPass = "";
	if (hasPassword)
	{
		if (!PromptPassword(password))
			return;
#ifdef _UNICODE
		if (WideCharToMultiByte(CP_ACP, 0, password, -1, passA, 129, NULL, NULL) == 0)
		{
			ShowMsgByACP(L"密码文本转换失败！");
			return;
		}
#else
		strncpy(passA, password, 128);
		passA[128] = '\0';
#endif
		pPass = passA;
	}

	if (!huffman.ReadCodeFromFile(savePath, pPass))
	{
		if (hasPassword)
			ShowMsgByACP(L"密码错误，或压缩文件已损坏！");
		else
			ShowMsgByACP(L"读取压缩文件失败，文件不存在或损坏！");
		return;
	}

	huffman.Decode();

	// 文件记录原文 CRC32 时，做解压结果一致性校验。
	if (storedCrc != 0)
	{
		unsigned long decodedCrc = huffman.GetTextCRC32();
		if (decodedCrc != storedCrc)
		{
			ShowMsgByACP(L"CRC 校验失败，解压结果与原文不一致！");
			return;
		}
	}

	char outPath[500] = {0};
	strncpy(outPath, savePath, sizeof(outPath) - 1);
	outPath[sizeof(outPath) - 1] = '\0';
	char* pExt = strrchr(outPath, '.');
	if (pExt != NULL)
		strcpy(pExt + 1, "txt");
	else
		strcat(outPath, ".txt");

	huffman.SaveTextToFile(outPath);
	ShowMsgByACP(L"解压完成。");
	EndDialog(IDOK);
}

// 功能：浏览并选择待解压 .huf 文件。
void CDecDlg::OnBrowse()
{
	// 代码段功能：打开文件对话框并把选中的路径回填到输入框。
	CString m_strFileName;
	CString szFilter = ToDialogTextByACP(L"压缩文件(*.huf)|*.huf|所有文件(*.*)|*.*||"); // 原文：压缩文件(*.huf)|*.huf|所有文件(*.*)|*.*||
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, (LPCTSTR)szFilter, this);

	if (fileDlg.DoModal() == IDOK)
	{
		m_strFileName = fileDlg.GetPathName();
		SetDlgItemText(IDC_EDIT2, m_strFileName);
		UpdateData(FALSE);
	}
}

// 功能：初始化解压设置对话框。
BOOL CDecDlg::OnInitDialog()
{
	// 代码段功能：初始化树控件内容。
	CDialog::OnInitDialog();

	HWND hWnd = m_treeCtrl.GetSafeHwnd();
	if (hWnd != NULL)
		InitTree();

	return TRUE;
}

// 功能：初始化目录树根节点。
void CDecDlg::InitTree()
{
	// 代码段功能：先插入根目录节点，后续可按需递归展开。
	HTREEITEM hRoot = m_treeCtrl.InsertItem(_T("D:\\"), TVI_ROOT, TVI_LAST);
	UNREFERENCED_PARAMETER(hRoot);
	// 若要展示完整目录结构，可启用以下递归调用。
	// PopulateTree(hRoot, _T("D:\\"));
}

// 功能：递归填充目录树。
void CDecDlg::PopulateTree(HTREEITEM hParent, LPCTSTR path)
{
	// 代码段功能：枚举当前目录并把文件/子目录加入树控件。
	CString fullPath;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	CString strPath(path);
	fullPath = strPath + _T("\\*.*");
	hFind = FindFirstFile(fullPath, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (FindFileData.cFileName[0] != '.')
			{
				CString subPath = strPath + _T("\\") + FindFileData.cFileName;
				HTREEITEM hChild = m_treeCtrl.InsertItem(FindFileData.cFileName, hParent, TVI_LAST);
				PopulateTree(hChild, subPath);
			}
		}
		else
		{
			m_treeCtrl.InsertItem(FindFileData.cFileName, hParent, TVI_LAST);
		}
	} while (FindNextFile(hFind, &FindFileData));

	FindClose(hFind);
}

// 功能：处理取消模式消息。
void CDecDlg::OnCancelMode()
{
	// 代码段功能：保持 CDialog 默认取消模式处理。
	CDialog::OnCancelMode();
}