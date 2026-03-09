// ComDlg.cpp : 压缩设置对话框实现
//

#include "stdafx.h"
#include "GoodYa.h"
#include "ComDlg.h"
#include "Huffman.h"
#include "PassDlg.h"
#include "zjh_codec.h"
#include <string>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// 功能：ToDialogTextByACP，将 Unicode 字符串转换为当前对话框编码。
static CString ToDialogTextByACP(const wchar_t* textW);
// 功能：ShowMsgByACP，按当前编码显示提示框文本。
static void ShowMsgByACP(const wchar_t* textW);

// 运行时创建控件 ID（避免修改 .rc/.h 资源号）。
// 说明：仅在 CComDlg 内部使用，不与现有资源 ID 冲突即可。
const int IDC_ZJH_PROGRESS_BAR_RUNTIME = 0x5A01;
const int IDC_ZJH_PROGRESS_TEXT_RUNTIME = 0x5A02;
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
	m_progressUiCreated = FALSE;
}

/**
 * 功能：绑定压缩设置对话框控件和成员变量。
 * 绑定对话框控件与类成员变量，执行 DDX/DDV 数据交换。
 * @param CDataExchange* pDX
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
void CComDlg::DoDataExchange(CDataExchange* pDX)
{
	// 代码段功能：执行 DDX/DDV 数据交换。
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

/**
 * 功能：处理“取消”按钮，关闭压缩设置对话框。
 * 返回 IDCANCEL 给调用方并关闭对话框。
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
void CComDlg::OnCancel()
{
	// 代码段功能：返回 IDCANCEL 给调用方。
	EndDialog(IDCANCEL);
}

/**
 * 读取输入路径并转换为 ANSI，便于当前 Huffman 接口使用。
 * 功能：获取待压缩文件路径并做编码转换与校验。
 * @param char outPath[500]
 * @return BOOL 转换成功返回 TRUE，失败返回 FALSE
 * @author zxl
 * @date 2024-06-01
 */
BOOL CComDlg::GetInputPath(char outPath[500])
{
	// 代码段功能：从编辑框读取路径并做编码转换与校验。
	if (outPath == NULL)
		return FALSE;

	TCHAR loadPathText[500] = {0};
	GetDlgItem(IDC_EDIT1)->GetWindowText(loadPathText, 500);

#ifdef _UNICODE
	if (WideCharToMultiByte(CP_ACP, 0, loadPathText, -1, outPath, 500, NULL, NULL) == 0)
	{
		AfxMessageBox(ToDialogTextByACP(L"路径转换失败！"));
		return FALSE;
	}
#else
	strncpy(outPath, loadPathText, 499);
	outPath[499] = '\0';
#endif

	if (outPath[0] == '\0')
	{
		AfxMessageBox(ToDialogTextByACP(L"请选择要压缩的文本文件！"));
		return FALSE;
	}
	return TRUE;
}

/**
 * 将密码转换为 ANSI 字节串，供 MD5 与加密流程使用。
 * 获取并转换密码为 ANSI。
 * @param char outPass[129]
 * @return BOOL 转换成功返回 TRUE，失败返回 FALSE
 * @author zxl
 * @date 2024-06-01
 */
BOOL CComDlg::GetPasswordA(char outPass[129])
{
	// 代码段功能：把当前对话框中的密码转换为 ANSI 字节。
	if (outPass == NULL)
		return FALSE;

	outPass[0] = '\0';
	if (m_password.IsEmpty())
		return TRUE;

#ifdef _UNICODE
	if (WideCharToMultiByte(CP_ACP, 0, m_password, -1, outPass, 129, NULL, NULL) == 0)
	{
		AfxMessageBox(ToDialogTextByACP(L"密码文本转换失败！"));
		return FALSE;
	}
#else
	strncpy(outPass, m_password, 128);
	outPass[128] = '\0';
#endif
	return TRUE;
}
/**
 * 功能：ToDialogTextByACP，把 Unicode 文本转换为当前进程使用的 TCHAR 文本。
 * 在非 Unicode 编译下按 ACP 编码转换，避免中文乱码。
 * @param const wchar_t* textW
 * @return CString 转换后的文本
 * @author zxl
 * @date 2024-06-01
 */
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

/**
 * 功能：ShowMsgByACP，统一消息框文本显示，避免直接弹框时中文乱码。
 * 通过统一转换后调用 AfxMessageBox。
 * @param const wchar_t* textW
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
static void ShowMsgByACP(const wchar_t* textW)
{
	// 代码段功能：通过统一转换后调用 AfxMessageBox。
	AfxMessageBox(ToDialogTextByACP(textW));
}

/**
 * 功能：把毫秒时长格式化为“时分秒”中文文本。
 * 论文说明：ETA 数值本质上是估计值，UI 只需提供量级可感知展示。
 * @param unsigned long milliseconds
 * @return CString
 */
CString CComDlg::FormatDurationText(unsigned long milliseconds) const
{
	unsigned long totalSeconds = milliseconds / 1000UL;
	unsigned long seconds = totalSeconds % 60UL;
	unsigned long minutesAll = totalSeconds / 60UL;
	unsigned long minutes = minutesAll % 60UL;
	unsigned long hours = minutesAll / 60UL;

	CString text;
	if (hours > 0)
		text.Format(_T("%lu小时%lu分%lu秒"), hours, minutes, seconds);
	else if (minutes > 0)
		text.Format(_T("%lu分%lu秒"), minutes, seconds);
	else
		text.Format(_T("%lu秒"), seconds);
	return text;
}

/**
 * 功能：确保进度条与进度文本控件已创建。
 * 设计理由：
 * 1) 采用运行时创建，避免修改 .rc 资源导致编码/布局回归风险；
 * 2) 同时服务 ZJH 与 HUF 两条压缩路径，避免维护两套显示控件。
 */
void CComDlg::EnsureProgressUi()
{
	if (m_progressUiCreated)
		return;

	CRect rcTextDU(35, 174, 146, 182);  // 以对话框单位描述，后续转像素
	CRect rcBarDU(35, 186, 146, 196);
	MapDialogRect(&rcTextDU);
	MapDialogRect(&rcBarDU);

	BOOL okText = m_zjhProgressText.Create(_T(""), WS_CHILD | WS_VISIBLE,
		rcTextDU, this, IDC_ZJH_PROGRESS_TEXT_RUNTIME);
	BOOL okBar = m_zjhProgress.Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
		rcBarDU, this, IDC_ZJH_PROGRESS_BAR_RUNTIME);
	if (!okText || !okBar)
	{
		m_progressUiCreated = FALSE;
		return;
	}

	m_zjhProgress.SetRange(0, 100);
	m_zjhProgress.SetPos(0);
	m_progressUiCreated = TRUE;
}

/**
 * 功能：重置进度显示为初始状态。
 * @return void
 */
void CComDlg::ResetProgressUi()
{
	EnsureProgressUi();
	if (!m_progressUiCreated)
		return;

	m_zjhProgress.SetPos(0);
	m_zjhProgressText.SetWindowText(ToDialogTextByACP(L"压缩进度：等待开始"));
}

/**
 * 功能：压缩开始后临时禁用关键输入控件，结束后恢复。
 * @param BOOL enabled
 * @return void
 */
void CComDlg::SetCompressionUiEnabled(BOOL enabled)
{
	const int controls[] = {
		IDC_OPEN, IDC_EDIT1, IDC_RADIO1, IDC_RADIO2,
		IDC_COMBO1, IDC_COMBO2, IDC_SETPASS,
		IDC_CHECK1, IDC_CHECK2, IDC_CHECK3,
		IDC_OK, IDC_CANCEL
	};

	for (int i = 0; i < (int)(sizeof(controls) / sizeof(controls[0])); ++i)
	{
		CWnd* pWnd = GetDlgItem(controls[i]);
		if (pWnd != NULL)
			pWnd->EnableWindow(enabled);
	}
}

/**
 * 功能：根据压缩回调实时刷新阶段、百分比、已耗时与 ETA。
 * 论文说明：为保证单线程压缩过程界面可刷新，此处主动泵消息队列。
 * @param int percent
 * @param int stage
 * @param unsigned long elapsedMs
 * @param unsigned long etaMs
 */
void CComDlg::UpdateProgressUi(int percent, int stage, unsigned long elapsedMs, unsigned long etaMs)
{
	EnsureProgressUi();
	if (!m_progressUiCreated)
		return;

	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;

	const wchar_t* stageText = L"准备中";
	switch (stage)
	{
	case HUF_PROGRESS_STAGE_PREPARE: stageText = L"准备压缩"; break;
	case HUF_PROGRESS_STAGE_READ_INPUT: stageText = L"读取源文件"; break;
	case HUF_PROGRESS_STAGE_COUNT_WEIGHT: stageText = L"统计字符频次"; break;
	case HUF_PROGRESS_STAGE_BUILD_TREE: stageText = L"构建哈夫曼树"; break;
	case HUF_PROGRESS_STAGE_ENCODE_DATA: stageText = L"生成编码数据"; break;
	case HUF_PROGRESS_STAGE_WRITE_FILE: stageText = L"写入压缩文件"; break;
	case HUF_PROGRESS_STAGE_DONE: stageText = L"压缩完成"; break;

	case ZJH_PROGRESS_STAGE_READ_INPUT: stageText = L"读取源文件"; break;
	case ZJH_PROGRESS_STAGE_BUILD_TREE: stageText = L"构建编码树"; break;
	case ZJH_PROGRESS_STAGE_OPTIMIZE_CODE: stageText = L"调整可重前缀"; break;
	case ZJH_PROGRESS_STAGE_PACK_OUTPUT: stageText = L"生成压缩数据"; break;
	case ZJH_PROGRESS_STAGE_ENCRYPT_PAYLOAD: stageText = L"加密压缩负载"; break;
	case ZJH_PROGRESS_STAGE_WRITE_FILE: stageText = L"写入压缩文件"; break;
	case ZJH_PROGRESS_STAGE_DONE: stageText = L"压缩完成"; break;
	default: stageText = L"准备中"; break;
	}

	m_zjhProgress.SetPos(percent);

	CString text;
	if (percent >= 100)
	{
		text.Format(_T("%s：100%%，总耗时 %s"),
			(LPCTSTR)ToDialogTextByACP(stageText),
			(LPCTSTR)FormatDurationText(elapsedMs));
	}
	else
	{
		text.Format(_T("%s：%d%%，已用 %s，预计剩余 %s"),
			(LPCTSTR)ToDialogTextByACP(stageText),
			percent,
			(LPCTSTR)FormatDurationText(elapsedMs),
			(LPCTSTR)FormatDurationText(etaMs));
	}
	m_zjhProgressText.SetWindowText(text);

	m_zjhProgress.UpdateWindow();
	m_zjhProgressText.UpdateWindow();
	UpdateWindow();

	MSG msg;
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

/**
 * 功能：zjh_codec 压缩进度回调静态转发函数。
 * @param int percent
 * @param int stage
 * @param unsigned long elapsedMs
 * @param unsigned long etaMs
 * @param void* userData
 */
void CComDlg::OnZjhProgress(int percent, int stage, unsigned long elapsedMs, unsigned long etaMs, void* userData)
{
	CComDlg* pThis = (CComDlg*)userData;
	if (pThis != NULL)
		pThis->UpdateProgressUi(percent, stage, elapsedMs, etaMs);
}

/**
 * 根据选中文件和压缩参数执行压缩，并按选项执行删除或 CRC 校验。
 * 功能：执行压缩流程。
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
void CComDlg::OnOk()
{
	// 代码段功能：同步界面状态、校验输入、执行压缩与可选校验。
	UpdateData(TRUE);
	ResetProgressUi();

	char loadPath[500] = {0};
	if (!GetInputPath(loadPath))
		return;

	if (m_isLock && m_password.IsEmpty())
	{
		ShowMsgByACP(L"已勾选加密压缩文件，请先设置密码！");
		return;
	}

	char passA[129] = {0};
	if (!GetPasswordA(passA))
		return;
	const char* pEncryptPass = m_isLock ? passA : "";

	if (m_Extend == 0)
	{
		// 代码段功能：执行 HUF（哈夫曼）压缩并计算压缩前 CRC32。
		Huffman huffman;
		huffman.SetProgressCallback(CComDlg::OnZjhProgress, this);
		SetCompressionUiEnabled(FALSE);
		huffman.ReadTextFromFile(loadPath);
		if (huffman.FileSize(loadPath) <= 0)
		{
			SetCompressionUiEnabled(TRUE);
			ShowMsgByACP(L"源文件为空，无法压缩！");
			return;
		}

		huffman.CountCharsWeight();
		huffman.MakeCharMap();
		huffman.Encode();
		unsigned long crcBefore = huffman.GetTextCRC32();

		char codePath[500] = {0};
		strncpy(codePath, loadPath, sizeof(codePath) - 1);
		codePath[sizeof(codePath) - 1] = '\0';
		strcat(codePath, ".huf");

		if (!huffman.SaveCodeToFile(codePath, pEncryptPass, crcBefore))
		{
			SetCompressionUiEnabled(TRUE);
			return;
		}

		// 代码段功能：勾选测试选项时，重读压缩包并比对 CRC32。
		if (m_isCheck)
		{
			Huffman verify;
			if (!verify.ReadCodeFromFile(codePath, pEncryptPass))
			{
				SetCompressionUiEnabled(TRUE);
				ShowMsgByACP(L"测试压缩文件失败，无法读取压缩包内容，校验失败！");
				return;
			}
			verify.Decode();
			unsigned long crcAfter = verify.GetTextCRC32();
			if (crcAfter != crcBefore)
			{
				SetCompressionUiEnabled(TRUE);
				ShowMsgByACP(L"CRC校验失败，压缩前后数据不一致！");
				return;
			}
		}

		SetCompressionUiEnabled(TRUE);
	}
	else
	{
		// 代码段功能：执行 ZJH 可重前缀压缩，并按“设置密码”选项决定是否加密。
		CComboBox* pMethod = (CComboBox*)GetDlgItem(IDC_COMBO2);
		int methodSel = (pMethod != NULL) ? pMethod->GetCurSel() : 1;
		int zjhLen = 8;
		if (methodSel == 0)
			zjhLen = 6;
		else if (methodSel == 2)
			zjhLen = 10;

		ZJH_encrypto1 zjh;
		SetCompressionUiEnabled(FALSE);
		if (!zjh.encrypto(std::string(loadPath),
			zjhLen,
			pEncryptPass,
			CComDlg::OnZjhProgress,
			this))
		{
			SetCompressionUiEnabled(TRUE);
			ShowMsgByACP(L"ZJH压缩失败，请检查源文件与压缩参数！");
			return;
		}

		char codePath[500] = {0};
		strncpy(codePath, loadPath, sizeof(codePath) - 1);
		codePath[sizeof(codePath) - 1] = '\0';
		strcat(codePath, ".zjh");

		// 代码段功能：勾选测试选项时，执行一次可解压校验（输出到临时文件并删除）。
		if (m_isCheck)
		{
			char verifyOut[500] = {0};
			strncpy(verifyOut, loadPath, sizeof(verifyOut) - 1);
			verifyOut[sizeof(verifyOut) - 1] = '\0';
			strcat(verifyOut, ".zjh.verify.tmp");

			ZJH_decrypto verify;
			if (!verify.decrypto_to(std::string(codePath), std::string(verifyOut), pEncryptPass))
			{
				SetCompressionUiEnabled(TRUE);
				ShowMsgByACP(L"测试压缩文件失败，ZJH压缩包无法正确解压！");
				return;
			}
			_unlink(verifyOut);
		}

		SetCompressionUiEnabled(TRUE);
	}

	// 代码段功能：若用户勾选删除源文件，执行删除并提示结果。
	if (m_isDelete)
	{
		if (_unlink(loadPath) != 0)
			ShowMsgByACP(L"提示：压缩完成，但删除源文件失败。");
	}

	ShowMsgByACP(L"压缩完成。");
	EndDialog(IDOK);
}

/**
 * 功能：向下拉框添加中文文本（含编码转换）。
 * 根据编译字符集选择直接添加或先转码再添加。
 * @param CComboBox* pCombo, const wchar_t* textW
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
static void AddComboTextByACP(CComboBox* pCombo, const wchar_t* textW)
{
	// 代码段功能：根据编译字符集选择直接添加或先转码再添加。
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

/**
 * 功能：初始化压缩设置界面中的下拉选项。
 * 填充压缩方式/更新方式并设置默认项和下拉宽度。
 * @return BOOL
 * @author zxl
 * @date 2024-06-01
 */
BOOL CComDlg::OnInitDialog()
{
	// 代码段功能：填充压缩方式/更新方式并设置默认项和下拉宽度。
	CDialog::OnInitDialog();

	// 初始化组合框内容，使用 Unicode 转 ACP，确保 GBK 环境可见。
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

	EnsureProgressUi();
	ResetProgressUi();

	return TRUE;
}

/**
 * 功能：处理取消模式消息。
 * 保持 CDialog 默认取消模式处理。
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
void CComDlg::OnCancelMode()
{
	// 代码段功能：保持 CDialog 默认取消模式处理。
	CDialog::OnCancelMode();
}

/**
 * 点击“设置密码”按钮，输入并更新压缩密码。
 * 功能：设置或清除压缩密码。
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
void CComDlg::OnSetpass()
{
	// 代码段功能：弹出密码输入框并更新“加密压缩文件”状态。
	CPassDlg dlg(
		ToDialogTextByACP(L"设置压缩密码"),
		ToDialogTextByACP(L"请输入压缩密码（留空表示清除密码）"),
		this);
	dlg.m_password = m_password;
	if (dlg.DoModal() == IDOK)
	{
		m_password = dlg.m_password;
		if (m_password.IsEmpty())
		{
			m_isLock = FALSE;
			ShowMsgByACP(L"已清除压缩密码。");
		}
		else
		{
			m_isLock = TRUE;
			ShowMsgByACP(L"压缩密码设置成功。");
		}
		UpdateData(FALSE);
	}
}

/**
 * 功能：浏览并选择待压缩文本文件。
 * 打开文件对话框并把选中的路径回填到输入框。
 * @return void
 * @author zxl
 * @date 2024-06-01
 */
void CComDlg::OnOpen()
{
	// 代码段功能：打开文件对话框并把选中的路径回填到输入框。
	CString m_strFileName;
	CString szFilter = ToDialogTextByACP(L"所有文件(*.*)|*.*|文本文件(*.txt)|*.txt||");
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, (LPCTSTR)szFilter, this);
	fileDlg.m_ofn.nFilterIndex = 1;

	if (fileDlg.DoModal() == IDOK)
	{
		m_strFileName = fileDlg.GetPathName();
		SetDlgItemText(IDC_EDIT1, m_strFileName);
	}
}
