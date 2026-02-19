// 压缩对话框实现文件
//

#include "stdafx.h"
#include "GoodYa.h"
#include "ComDlg.h"
#include "Huffman.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// 压缩对话框


CComDlg::CComDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CComDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CComDlg)
	m_Extend = 0;
	m_isDelete = FALSE;
	m_isCheck = TRUE;
	m_isLock = FALSE;
	//}}AFX_DATA_INIT
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
// 压缩对话框消息处理

void CComDlg::OnCancel() 
{
	// 待完善：在此添加控件通知处理程序代码
	//CDialog::OnCancel();	
	EndDialog(IDOK);
}

// 对选中文件进行压缩
void CComDlg::OnOk() 
{
	// 待完善：在此添加控件通知处理程序代码
	Huffman huffman;
	TCHAR loadPathText[500] = {0};
	char loadPath[500] = {0};
	char codePath[500] = {0};
	char* codepath;

	// 从界面读取待压缩文件路径
	GetDlgItem(IDC_EDIT1)->GetWindowText(loadPathText,500);
#ifdef _UNICODE
	if (WideCharToMultiByte(CP_ACP, 0, loadPathText, -1, loadPath, sizeof(loadPath), NULL, NULL) == 0)
	{
		AfxMessageBox(_T("路径转换失败！"));
		return;
	}
#else
	strncpy(loadPath, loadPathText, sizeof(loadPath) - 1);
	loadPath[sizeof(loadPath) - 1] = '\0';
#endif
	if (loadPath[0] == '\0')
	{
		AfxMessageBox(_T("请选择 .txt 文件！"));
		return;
	}
	// 读取原文
	huffman.ReadTextFromFile(loadPath);
	// 统计字符权值
	huffman.CountCharsWeight();
	// 建立字符-编码映射
	huffman.MakeCharMap();
	// 执行哈夫曼编码
	huffman.Encode();

	// 生成输出 .huf 文件路径
	int loadLen = (int)strlen(loadPath);
	if (loadLen < 3)
	{
		AfxMessageBox(_T("源文件路径无效！"));
		return;
	}
	strncpy(codePath,loadPath,loadLen-3);
	codePath[loadLen-3] = '\0';
	codepath = strcat(codePath,"huf");
	// 保存编码结果
	huffman.SaveCodeToFile(codepath);
	MessageBox(_T("压缩完成。"));
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

static void SetComboTextByACP(CComboBox* pCombo, const wchar_t* textW)
{
	if (pCombo == NULL || textW == NULL)
		return;
#ifdef _UNICODE
	pCombo->SetWindowText(textW);
#else
	char textA[128] = {0};
	if (WideCharToMultiByte(CP_ACP, 0, textW, -1, textA, sizeof(textA), NULL, NULL) > 0)
		pCombo->SetWindowText(textA);
#endif
}
BOOL CComDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// 待完善：在此添加额外初始化
	CComboBox* pMethod = (CComboBox*)GetDlgItem(IDC_COMBO2);
	CComboBox* pUpdate = (CComboBox*)GetDlgItem(IDC_COMBO1);

	if (pMethod != NULL)
	{
		pMethod->ResetContent();
		AddComboTextByACP(pMethod, L"\x6700\x5FEB");	// 最快
		AddComboTextByACP(pMethod, L"\x6807\x51C6");	// 标准
		AddComboTextByACP(pMethod, L"\x6700\x597D");	// 最好
		if (pMethod->SetCurSel(1) == CB_ERR && pMethod->GetCount() > 0)
			pMethod->SetCurSel(0);
		pMethod->SetDroppedWidth(120);
	}

	if (pUpdate != NULL)
	{
		pUpdate->ResetContent();
		AddComboTextByACP(pUpdate, L"\x66FF\x6362\x5DF2\x5B58\x5728\x6587\x4EF6");	// 替换已存在文件
		AddComboTextByACP(pUpdate, L"\x8986\x76D6\x524D\x8BE2\x95EE");		// 覆盖前询问
		AddComboTextByACP(pUpdate, L"\x8DF3\x8FC7\x5DF2\x5B58\x5728\x6587\x4EF6");	// 跳过已存在文件
		if (pUpdate->SetCurSel(1) == CB_ERR && pUpdate->GetCount() > 0)
			pUpdate->SetCurSel(0);
		pUpdate->SetDroppedWidth(220);
	}

	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
	              // 例外: OCX 属性页应返回 FALSE
}


void CComDlg::OnCancelMode() 
{
	CDialog::OnCancelMode();
	
	// 待完善：在此添加消息处理程序代码
	
}

void CComDlg::OnSetpass() 
{
	// 待完善：在此添加控件通知处理程序代码
	
}

void CComDlg::OnOpen() 
{
	CString m_strFileName;
	// 待完善：在此添加控件通知处理程序代码
	// *.exe 表示仅显示 exe 文件，*.* 表示显示所有文件
	LPCTSTR szFilter = _T("文本文件(*.txt)|*.txt|所有文件(*.*)|*.*||");
	// 显示打开文件对话框
	CFileDialog fileDlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,szFilter,NULL);

	// 用户点击 OK 后，将路径写入编辑框
	if(fileDlg.DoModal()==IDOK)
    {
		m_strFileName=fileDlg.GetPathName();
		SetDlgItemText(IDC_EDIT1,m_strFileName);
		// 同步变量到控件显示
		UpdateData(FALSE);
    }  
}
