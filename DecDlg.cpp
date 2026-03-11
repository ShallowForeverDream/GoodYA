// DecDlg.cpp : 解压设置对话框实现
//

#include "stdafx.h"
#include "GoodYa.h"
#include "DecDlg.h"
#include "Huffman.h"
#include "PassDlg.h"
#include "zjh_codec.h"
#include <string>
#include <string.h>
#include <shellapi.h>

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

// 功能：判断 ANSI 路径是否以指定后缀结尾（不区分大小写）。
static BOOL EndsWithIgnoreCase(const char* text, const char* suffix)
{
	if (text == NULL || suffix == NULL)
		return FALSE;

	int textLen = (int)strlen(text);
	int suffixLen = (int)strlen(suffix);
	if (textLen < suffixLen)
		return FALSE;

	const char* p = text + (textLen - suffixLen);
	for (int i = 0; i < suffixLen; ++i)
	{
		char a = p[i];
		char b = suffix[i];
		if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
		if (a != b)
			return FALSE;
	}

	return TRUE;
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
	// 注册树控件的展开与选择变化通知，便于动态填充与回填路径
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE1, OnTvnItemExpanding)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, OnTvnSelchangedTree1)
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
		ShowMsgByACP(L"请选择 .huf 或 .zjh 文件！");
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
	try
	{
		char savePath[500] = {0};
		if (!GetInputPath(savePath))
			return;

		BOOL isHuf = EndsWithIgnoreCase(savePath, ".huf");
		BOOL isZjh = EndsWithIgnoreCase(savePath, ".zjh");
		if (!isHuf && !isZjh)
		{
			ShowMsgByACP(L"仅支持解压 .huf 或 .zjh 文件！");
			return;
		}

		Huffman huffman;
		BOOL hasPassword = FALSE;
		unsigned long storedCrc = 0;
		char md5Hex[33] = {0};

		if (isHuf)
		{
			if (!huffman.GetPackageInfo(savePath, &hasPassword, &storedCrc, md5Hex))
			{
				ShowMsgByACP(L"读取压缩文件信息失败！");
				return;
			}
		}
		else
		{
			bool hasPasswordZjh = false;
			if (!ZJH_GetPackageInfo(std::string(savePath), &hasPasswordZjh))
			{
				ShowMsgByACP(L"读取 ZJH 文件信息失败！");
				return;
			}
			hasPassword = hasPasswordZjh ? TRUE : FALSE;
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

		if (isHuf)
		{
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
		}
		else
		{
			ZJH_decrypto decrypto;
			if (!decrypto.decrypto(std::string(savePath), pPass))
			{
				if (hasPassword)
					ShowMsgByACP(L"ZJH 解压失败：密码错误，或压缩包已损坏！");
				else
					ShowMsgByACP(L"ZJH 解压失败：文件格式不正确、内容损坏、无写权限或磁盘空间不足！");
				return;
			}
		}

		ShowMsgByACP(L"解压完成。");
		EndDialog(IDOK);
	}
	catch (CMemoryException* e)
	{
		if (e != NULL) e->Delete();
		ShowMsgByACP(L"解压失败：内存不足，请检查输出目录磁盘空间后重试。");
	}
	catch (...)
	{
		ShowMsgByACP(L"解压失败：文件可能损坏、密码错误、无写权限或磁盘空间不足。");
	}
}
// 功能：浏览并选择待解压 .huf/.zjh 文件。
void CDecDlg::OnBrowse()
{
	// 代码段功能：打开文件对话框并把选中的路径回填到输入框。
	CString m_strFileName;
	CString szFilter = ToDialogTextByACP(L"压缩文件(*.huf;*.zjh)|*.huf;*.zjh|HUF文件(*.huf)|*.huf|ZJH文件(*.zjh)|*.zjh|所有文件(*.*)|*.*||"); // 原文：压缩文件(*.huf;*.zjh)|*.huf;*.zjh|所有文件(*.*)|*.*||
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

	// 增强树控件显示：启用层级线、展开按钮，并设置用于区分文件/文件夹的图标列表
	HWND hWnd = m_treeCtrl.GetSafeHwnd();
	if (hWnd != NULL)
	{
		// 增加树的样式以显示连接线和展开按钮（增强层次感）
		m_treeCtrl.ModifyStyle(0, TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS);

		// 不使用图标：树仅显示文本，取消创建/绑定图像列表

		// 初始化根节点（并为根填充一级子项）
		InitTree();
	}

	return TRUE;
}

// 功能：初始化目录树根节点。
void CDecDlg::InitTree()
{
	// 代码段功能：先插入根目录节点，后续可按需递归展开。
	// 插入根节点并惰性填充一级子项（每个文件夹加入占位子项以显示可展开标志）
		HTREEITEM hRoot = m_treeCtrl.InsertItem(_T("D:\\"), TVI_ROOT, TVI_LAST);
		// 只填充根目录的第一层子项，避免一次性加载大量文件导致界面卡顿
		PopulateTree(hRoot, _T("D:\\"));
}

// 功能：递归填充目录树。
void CDecDlg::PopulateTree(HTREEITEM hParent, LPCTSTR path)
{
	// 代码段功能：惰性填充当前目录的一级子项。
	// 对于子目录，仅插入一个空的占位子项以显示“可展开”符号，实际内容在展开时再枚举。
	CString searchPath;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	CString strPath(path);
	searchPath = strPath + _T("\\*.*");
	hFind = FindFirstFile(searchPath, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		// 忽略 '.' 和 '..' 项
		if (FindFileData.cFileName[0] == '.')
			continue;

		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			HTREEITEM hChild = m_treeCtrl.InsertItem(FindFileData.cFileName, hParent, TVI_LAST);
			// 插入占位子项以显示可展开按钮
			m_treeCtrl.InsertItem(_T(""), hChild, TVI_LAST); // 占位
		}
		else
		{
			// 插入文件节点（无子项），不使用图标
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

// 功能：当用户展开某一节点时（TVN_ITEMEXPANDING），按需填充该节点的子项
// 注：此处采用惰性填充策略——只有当节点没有子项时才枚举文件系统并插入子项，
// 以避免一次性加载整个磁盘目录造成界面卡顿。所有中文注释使用 GBK 假定编码说明业务意图。
void CDecDlg::OnTvnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREEVIEW* pNMTV = reinterpret_cast<NMTREEVIEW*>(pNMHDR);
	if (pNMTV == NULL)
	{
		*pResult = 0;
		return;
	}

	// 只在展开时填充（避免折叠时再次枚举）
	if (pNMTV->action == TVE_EXPAND)
	{
		HTREEITEM hItem = pNMTV->itemNew.hItem;
		if (hItem == NULL)
		{
			*pResult = 0;
			return;
		}

		// 如果该节点已有子节点且第一个子项不是占位，则认为已填充过，跳过枚举
		HTREEITEM hFirstChild = m_treeCtrl.GetChildItem(hItem);
		if (hFirstChild == NULL)
		{
			*pResult = 0;
			return;
		}

		CString firstText = m_treeCtrl.GetItemText(hFirstChild);
		if (!firstText.IsEmpty())
		{
			// 已经填充过真实子项，直接返回
			*pResult = 0;
			return;
		}

		// 此时第一个子项为占位项，先删除所有占位子项，然后枚举目录真实内容并插入
		// 删除所有子项占位
		while (m_treeCtrl.GetChildItem(hItem) != NULL)
		{
			HTREEITEM hDel = m_treeCtrl.GetChildItem(hItem);
			m_treeCtrl.DeleteItem(hDel);
		}

		// 从当前节点向上遍历构建完整路径
		CStringArray parts;
		HTREEITEM h = hItem;
		while (h != NULL)
		{
			CString txt = m_treeCtrl.GetItemText(h);
			parts.InsertAt(0, txt);
			h = m_treeCtrl.GetParentItem(h);
		}

		if (parts.GetSize() == 0)
		{
			*pResult = 0;
			return;
		}

		// 合并路径段，注意根节点可能已经包含反斜杠
		CString fullPath = parts[0];
		for (int i = 1; i < parts.GetSize(); ++i)
		{
			if (fullPath.Right(1) == "\\" || fullPath.Right(1) == ":")
				fullPath += parts[i];
			else
				fullPath += "\\" + parts[i];
		}

		// 枚举 fullPath 下的一级子项并插入（目录插入占位子项）
		WIN32_FIND_DATA FindFileData;
		CString searchPath = fullPath + _T("\\*.*");
		HANDLE hFind = FindFirstFile(searchPath, &FindFileData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (FindFileData.cFileName[0] == '.')
					continue;

				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					HTREEITEM hChild = m_treeCtrl.InsertItem(FindFileData.cFileName, hItem, TVI_LAST);
					m_treeCtrl.InsertItem(_T(""), hChild, TVI_LAST); // 占位
				}
				else
				{
					// 文件：插入但不设置图标
					m_treeCtrl.InsertItem(FindFileData.cFileName, hItem, TVI_LAST);
				}
			} while (FindNextFile(hFind, &FindFileData));

			FindClose(hFind);
		}
	}

	*pResult = 0;
}

// 功能：当树选择改变时，把选中项的完整路径回填至目标路径编辑框（IDC_EDIT2）
// 行为：无论选中的是文件还是目录，都会把完整路径写入目标路径输入框，方便用户直接确认或修改。
void CDecDlg::OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTREEVIEW* pNMTV = reinterpret_cast<NMTREEVIEW*>(pNMHDR);
	if (pNMTV == NULL)
	{
		*pResult = 0;
		return;
	}

	HTREEITEM hItem = pNMTV->itemNew.hItem;
	if (hItem == NULL)
	{
		*pResult = 0;
		return;
	}

	// 构造从根到当前节点的路径段数组
	CStringArray parts;
	HTREEITEM h = hItem;
	while (h != NULL)
	{
		CString txt = m_treeCtrl.GetItemText(h);
		parts.InsertAt(0, txt);
		h = m_treeCtrl.GetParentItem(h);
	}

	if (parts.GetSize() == 0)
	{
		*pResult = 0;
		return;
	}

	CString fullPath = parts[0];
	for (int i = 1; i < parts.GetSize(); ++i)
	{
		if (fullPath.Right(1) == "\\" || fullPath.Right(1) == ":")
			fullPath += parts[i];
		else
			fullPath += "\\" + parts[i];
	}

	// 判断当前选中项是否为目录：若有子项则视为目录（我们为目录插入了占位子项），点击目录则展开/折叠，
	// 点击文件则回填完整路径到目标路径编辑框
	if (m_treeCtrl.GetChildItem(hItem) != NULL)
	{
		// 目录：切换展开/折叠状态
		UINT state = m_treeCtrl.GetItemState(hItem, TVIS_EXPANDED);
		if (state & TVIS_EXPANDED)
			m_treeCtrl.Expand(hItem, TVE_COLLAPSE);
		else
			m_treeCtrl.Expand(hItem, TVE_EXPAND);
	}
	else
	{
		// 文件：回填到目标路径编辑框
		SetDlgItemText(IDC_EDIT2, fullPath);
	}

	*pResult = 0;
}


