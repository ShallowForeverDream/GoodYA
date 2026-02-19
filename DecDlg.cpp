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

void CDecDlg::DoDataExchange(CDataExchange* pDX)
{
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

void CDecDlg::OnDecDlgCancel()
{
	EndDialog(IDCANCEL);
}

// 读取输入路径并转成 ANSI，兼容 Huffman 接口
BOOL CDecDlg::GetInputPath(char outPath[500])
{
	if (outPath == NULL)
		return FALSE;

	TCHAR savePathText[500] = {0};
	GetDlgItem(IDC_EDIT2)->GetWindowText(savePathText, 500);

#ifdef _UNICODE
	if (WideCharToMultiByte(CP_ACP, 0, savePathText, -1, outPath, 500, NULL, NULL) == 0)
	{
		AfxMessageBox(_T("路径转换失败！"));
		return FALSE;
	}
#else
	strncpy(outPath, savePathText, 499);
	outPath[499] = '\0';
#endif

	if (outPath[0] == '\0')
	{
		AfxMessageBox(_T("请选择 .huf 文件！"));
		return FALSE;
	}
	return TRUE;
}

// 弹出密码输入框，供解压时验证口令
BOOL CDecDlg::PromptPassword(CString& outPassword)
{
	CPassDlg dlg(_T("输入解压密码"), _T("该压缩文件已加密，请输入密码："), this);
	if (dlg.DoModal() != IDOK)
		return FALSE;

	outPassword = dlg.m_password;
	if (outPassword.IsEmpty())
	{
		AfxMessageBox(_T("密码不能为空！"));
		return FALSE;
	}
	return TRUE;
}

// 对文件进行解压缩，支持密码校验与 CRC 校验
void CDecDlg::OnDecDlgOk()
{
	char savePath[500] = {0};
	if (!GetInputPath(savePath))
		return;

	Huffman huffman;
	BOOL hasPassword = FALSE;
	unsigned long storedCrc = 0;
	char md5Hex[33] = {0};
	if (!huffman.GetPackageInfo(savePath, &hasPassword, &storedCrc, md5Hex))
	{
		AfxMessageBox(_T("读取压缩文件信息失败！"));
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
			AfxMessageBox(_T("密码编码转换失败！"));
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
			AfxMessageBox(_T("密码错误，或压缩文件已损坏！"));
		else
			AfxMessageBox(_T("读取压缩文件失败，文件可能已损坏！"));
		return;
	}

	huffman.Decode();

	// 若文件记录了原文 CRC32，则在解压后进行一致性校验
	if (storedCrc != 0)
	{
		unsigned long decodedCrc = huffman.GetTextCRC32();
		if (decodedCrc != storedCrc)
		{
			AfxMessageBox(_T("CRC 校验失败：解压结果与原文不一致！"));
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
	AfxMessageBox(_T("解压完成。"));
	EndDialog(IDOK);
}

void CDecDlg::OnBrowse()
{
	CString m_strFileName;
	LPCTSTR szFilter = _T("压缩文件(*.huf)|*.huf|所有文件(*.*)|*.*||");
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, this);

	if (fileDlg.DoModal() == IDOK)
	{
		m_strFileName = fileDlg.GetPathName();
		SetDlgItemText(IDC_EDIT2, m_strFileName);
		UpdateData(FALSE);
	}
}

BOOL CDecDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	HWND hWnd = m_treeCtrl.GetSafeHwnd();
	if (hWnd != NULL)
		InitTree();

	return TRUE;
}

void CDecDlg::InitTree()
{
	HTREEITEM hRoot = m_treeCtrl.InsertItem(_T("D:\\"), TVI_ROOT, TVI_LAST);
	UNREFERENCED_PARAMETER(hRoot);
	// 如需展示完整目录树，可启用下方递归函数
	// PopulateTree(hRoot, _T("D:\\"));
}

void CDecDlg::PopulateTree(HTREEITEM hParent, LPCTSTR path)
{
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

void CDecDlg::OnCancelMode()
{
	CDialog::OnCancelMode();
}