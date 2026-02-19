// ComDlg.cpp : 压缩设置对话框实现
//

#include "stdafx.h"
#include "GoodYa.h"
#include "ComDlg.h"
#include "Huffman.h"
#include "PassDlg.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComDlg 对话框

CComDlg::CComDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CComDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CComDlg)
	m_Extend = 0;
	m_isDelete = FALSE;
	m_isCheck = TRUE;
	m_isLock = FALSE;
	//}}AFX_DATA_INIT
	m_password = _T("");
}

void CComDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CComDlg)
	DDX_Radio(pDX, IDC_RADIO1, m_Extend);
	DDX_Check(pDX, IDC_CHECK1, m_isDelete);
	DDX_Check(pDX, IDC_CHECK2, m_isCheck);
	DDX_Check(pDX, IDC_CHECK3, m_isLock);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CComDlg, CDialog)
	//{{AFX_MSG_MAP(CComDlg)
	ON_BN_CLICKED(IDC_OK, OnOk)
	ON_WM_CANCELMODE()
	ON_BN_CLICKED(IDC_SETPASS, OnSetpass)
	ON_BN_CLICKED(IDC_CANCEL, OnCancel)
	ON_BN_CLICKED(IDC_OPEN, OnOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CComDlg 消息处理

void CComDlg::OnCancel()
{
	EndDialog(IDCANCEL);
}

// 读取输入路径并转成 ANSI，兼容当前 Huffman 接口
BOOL CComDlg::GetInputPath(char outPath[500])
{
	if (outPath == NULL)
		return FALSE;

	TCHAR loadPathText[500] = {0};
	GetDlgItem(IDC_EDIT1)->GetWindowText(loadPathText, 500);

#ifdef _UNICODE
	if (WideCharToMultiByte(CP_ACP, 0, loadPathText, -1, outPath, 500, NULL, NULL) == 0)
	{
		AfxMessageBox(_T("路径转换失败！"));
		return FALSE;
	}
#else
	strncpy(outPath, loadPathText, 499);
	outPath[499] = '\0';
#endif

	if (outPath[0] == '\0')
	{
		AfxMessageBox(_T("请选择要压缩的文本文件！"));
		return FALSE;
	}
	return TRUE;
}

// 将密码转换为 ANSI 字节串，供 MD5 和加密使用
BOOL CComDlg::GetPasswordA(char outPass[129])
{
	if (outPass == NULL)
		return FALSE;

	outPass[0] = '\0';
	if (m_password.IsEmpty())
		return TRUE;

#ifdef _UNICODE
	if (WideCharToMultiByte(CP_ACP, 0, m_password, -1, outPass, 129, NULL, NULL) == 0)
	{
		AfxMessageBox(_T("密码编码转换失败！"));
		return FALSE;
	}
#else
	strncpy(outPass, m_password, 128);
	outPass[128] = '\0';
#endif
	return TRUE;
}

// 对选中文件进行压缩，并按选项执行密码与 CRC 校验
void CComDlg::OnOk()
{
	UpdateData(TRUE);

	char loadPath[500] = {0};
	if (!GetInputPath(loadPath))
		return;

	if (m_isLock && m_password.IsEmpty())
	{
		AfxMessageBox(_T("已勾选锁定压缩文件，请先设置密码！"));
		return;
	}

	Huffman huffman;
	huffman.ReadTextFromFile(loadPath);
	if (huffman.FileSize(loadPath) <= 0)
	{
		AfxMessageBox(_T("源文件为空，无法压缩！"));
		return;
	}

	huffman.CountCharsWeight();
	huffman.MakeCharMap();
	huffman.Encode();

	unsigned long crcBefore = huffman.GetTextCRC32();

	char codePath[500] = {0};
	strncpy(codePath, loadPath, sizeof(codePath) - 1);
	codePath[sizeof(codePath) - 1] = '\0';
	char* pExt = strrchr(codePath, '.');
	if (pExt != NULL)
		strcpy(pExt + 1, "huf");
	else
		strcat(codePath, ".huf");

	char passA[129] = {0};
	if (!GetPasswordA(passA))
		return;

	const char* pEncryptPass = m_isLock ? passA : "";
	if (!huffman.SaveCodeToFile(codePath, pEncryptPass, crcBefore))
		return;

	// 勾选“测试压缩文件”时，立即重读+解压并做 CRC32 对比
	if (m_isCheck)
	{
		Huffman verify;
		if (!verify.ReadCodeFromFile(codePath, pEncryptPass))
		{
			AfxMessageBox(_T("测试压缩文件失败：无法读取压缩包或密码校验失败！"));
			return;
		}
		verify.Decode();
		unsigned long crcAfter = verify.GetTextCRC32();
		if (crcAfter != crcBefore)
		{
			AfxMessageBox(_T("CRC 校验失败：压缩前后内容不一致！"));
			return;
		}
	}

	if (m_isDelete)
	{
		if (_unlink(loadPath) != 0)
			AfxMessageBox(_T("提示：压缩完成，但删除源文件失败。"));
	}

	AfxMessageBox(_T("压缩完成。"));
	EndDialog(IDOK);
}

static void AddComboTextByACP(CComboBox* pCombo, const wchar_t* textW)
{
	if (pCombo == NULL || textW == NULL)
		return;
#ifdef _UNICODE
	pCombo->AddString(textW);
#else
	char textA[128] = {0};
	if (WideCharToMultiByte(CP_ACP, 0, textW, -1, textA, sizeof(textA), NULL, NULL) > 0)
		pCombo->AddString(textA);
#endif
}

BOOL CComDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 初始化下拉框内容（使用 Unicode 转 ACP，确保 GBK 界面可见）
	CComboBox* pMethod = (CComboBox*)GetDlgItem(IDC_COMBO2);
	CComboBox* pUpdate = (CComboBox*)GetDlgItem(IDC_COMBO1);

	if (pMethod != NULL)
	{
		pMethod->ResetContent();
		AddComboTextByACP(pMethod, L"最快"); // 最快
		AddComboTextByACP(pMethod, L"标准"); // 标准
		AddComboTextByACP(pMethod, L"最好"); // 最好
		if (pMethod->SetCurSel(1) == CB_ERR && pMethod->GetCount() > 0)
			pMethod->SetCurSel(0);
		pMethod->SetDroppedWidth(120);
	}

	if (pUpdate != NULL)
	{
		pUpdate->ResetContent();
		AddComboTextByACP(pUpdate, L"替换已存在文件"); // 替换已存在文件
		AddComboTextByACP(pUpdate, L"覆盖前询问");     // 覆盖前询问
		AddComboTextByACP(pUpdate, L"跳过已存在文件"); // 跳过已存在文件
		if (pUpdate->SetCurSel(1) == CB_ERR && pUpdate->GetCount() > 0)
			pUpdate->SetCurSel(0);
		pUpdate->SetDroppedWidth(220);
	}

	return TRUE;
}

void CComDlg::OnCancelMode()
{
	CDialog::OnCancelMode();
}

// 点击“设置密码”，输入并缓存压缩密码
void CComDlg::OnSetpass()
{
	CPassDlg dlg(_T("设置压缩密码"), _T("请输入压缩密码（留空表示清除密码）："), this);
	dlg.m_password = m_password;
	if (dlg.DoModal() == IDOK)
	{
		m_password = dlg.m_password;
		if (m_password.IsEmpty())
		{
			m_isLock = FALSE;
			AfxMessageBox(_T("已清除压缩密码。"));
		}
		else
		{
			m_isLock = TRUE;
			AfxMessageBox(_T("压缩密码设置成功。"));
		}
		UpdateData(FALSE);
	}
}

void CComDlg::OnOpen()
{
	CString m_strFileName;
	LPCTSTR szFilter = _T("文本文件(*.txt)|*.txt|所有文件(*.*)|*.*||");
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, NULL);

	if (fileDlg.DoModal() == IDOK)
	{
		m_strFileName = fileDlg.GetPathName();
		SetDlgItemText(IDC_EDIT1, m_strFileName);
		UpdateData(FALSE);
	}
}