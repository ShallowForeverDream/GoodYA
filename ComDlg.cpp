// ComDlg.cpp : implementation file
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
// CComDlg dialog


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
// CComDlg message handlers

void CComDlg::OnCancel() 
{
	// TODO: Add your control notification handler code here
	//CDialog::OnCancel();	
	EndDialog(IDOK);
}

//对文件进行压缩
void CComDlg::OnOk() 
{
	// TODO: Add your control notification handler code here
	Huffman huffman;
	char loadPath[500],codePath[500],len[10];
	char* codepath;

	//获取待压缩文件路径
	GetDlgItem(IDC_EDIT1)->GetWindowText(loadPath,500);
	//MessageBox(loadPath);
	//从文件读入原文
	huffman.ReadTextFromFile(loadPath);
	//统计原文中各字符的权值
	huffman.CountCharsWeight();
	//根据各字符的权值建立字符-编码表
	huffman.MakeCharMap();
	//对Text串进行哈夫曼编码
	huffman.Encode();
	//设置压缩文件存储路径以及文件名
	strncpy(codePath,loadPath,strlen(loadPath)-3);
	codePath[strlen(loadPath)-3] = '\0';
	codepath = strcat(codePath,"huf");
	//MessageBox(codepath);
	//将编码存入文件
	huffman.SaveCodeToFile(codepath);
	MessageBox("文件压缩成功！");
}

BOOL CComDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	((CComboBox*)GetDlgItem(IDC_COMBO2))->AddString("最快");
	((CComboBox*)GetDlgItem(IDC_COMBO2))->AddString("标准");
	((CComboBox*)GetDlgItem(IDC_COMBO2))->AddString("最好");
	((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(1);

	((CComboBox*)GetDlgItem(IDC_COMBO1))->AddString("替换已存在文件");
	((CComboBox*)GetDlgItem(IDC_COMBO1))->AddString("覆盖前询问");
	((CComboBox*)GetDlgItem(IDC_COMBO1))->AddString("跳过已存在文件");
	((CComboBox*)GetDlgItem(IDC_COMBO1))->SetCurSel(1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CComDlg::OnCancelMode() 
{
	CDialog::OnCancelMode();
	
	// TODO: Add your message handler code here
	
}

void CComDlg::OnSetpass() 
{
	// TODO: Add your control notification handler code here
	
}

void CComDlg::OnOpen() 
{
	CString m_strFileName;
	// TODO: Add your control notification handler code here
	// *.exe 表示只打开exe文件， *.* 表示所有文件
	char szFilter[] = {"txt files(*.txt)|*.txt|All Files(*.*)|*.*|"};
	//显示打开文件的对话框
	CFileDialog fileDlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,szFilter,NULL);

	//当用户选择OK时，程序取得选择文件的全路径名（包括文件的路径及文件名称），并将相应的数值传输给相关的控件变量。
	if(fileDlg.DoModal()==IDOK)
    {
		m_strFileName=fileDlg.GetPathName();
		SetDlgItemText(IDC_EDIT1,m_strFileName);
		//向将变量中的数值传输给控件显示出来。
		UpdateData(FALSE);
    }  
}
