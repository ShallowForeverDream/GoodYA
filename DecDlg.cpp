// DecDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GoodYa.h"
#include "DecDlg.h"
#include "Huffman.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDecDlg dialog


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
// CDecDlg message handlers

void CDecDlg::OnDecDlgCancel() 
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();	
}

//对文件进行解压缩
void CDecDlg::OnDecDlgOk() 
{
	// TODO: Add your control notification handler code here
	Huffman huffman;
	char savePath[500],codePath[500];
	char* savepath;

	//获取待解压文件路径
	GetDlgItem(IDC_EDIT2)->GetWindowText(savePath,500);
	//MessageBox(savePath);
	//从文件读入编码
	huffman.ReadCodeFromFile(savePath);
	//将0-1编码串解码
	huffman.Decode();
	huffman.PrintCode();
	//设置解压文件存储路径以及文件名
	strncpy(codePath,savePath,strlen(savePath)-3);
	codePath[strlen(savePath)-3] = '\0';
	savepath = strcat(codePath,"txt");
	//MessageBox(savepath);
	//将解码结果存入文件
	huffman.SaveTextToFile(savepath);
	MessageBox("文件解压成功！");
}

void CDecDlg::OnBrowse() 
{
	// TODO: Add your control notification handler code here
	CString m_strFileName;
	// TODO: Add your control notification handler code here
	// *.exe 表示只打开exe文件， *.* 表示所有文件
	char szFilter[] = {"huf files(*.huf)|*.huf|All Files(*.*)|*.*|"};
	//显示打开文件的对话框
	CFileDialog fileDlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,szFilter,NULL);

	//当用户选择OK时，程序取得选择文件的全路径名（包括文件的路径及文件名称），并将相应的数值传输给相关的控件变量。
	if(fileDlg.DoModal()==IDOK)
    {
		m_strFileName=fileDlg.GetPathName();
		SetDlgItemText(IDC_EDIT2,m_strFileName);
		//向将变量中的数值传输给控件显示出来。
		UpdateData(FALSE);
    }  
	
}

BOOL CDecDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
    // 获取树形控件的句柄
	HWND hWnd = m_treeCtrl.GetSafeHwnd();
	if (hWnd != NULL) {
		// 使用hWnd进行操作
	    // 初始化树形控件
		InitTree();
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDecDlg::InitTree()
{
	// 根节点为C盘根目录
    HTREEITEM hRoot = m_treeCtrl.InsertItem(_T("D:\\"), TVI_ROOT, TVI_LAST); 
	// 递归填充子目录和文件
    //PopulateTree(hRoot, _T("D:\\")); 
}

void CDecDlg::PopulateTree(HTREEITEM hParent, LPCTSTR path){
    CString fullPath;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
	// 先转换成 CString
    CString strPath(path);
	// 获取目录下所有文件和子目录的路径
    fullPath = strPath + _T("\\*.*");
	// 开始查找第一个匹配项
    hFind = FindFirstFile(fullPath, &FindFileData);
	// 查找失败，返回
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
		// 是目录则递归添加子目录和文件
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// 忽略隐藏目录（如 . 和 ..）
            if (FindFileData.cFileName[0] != '.') {
				// 子目录完整路径
                CString subPath = strPath + _T("\\") + FindFileData.cFileName;
				// 添加子目录项到树中
                HTREEITEM hChild = m_treeCtrl.InsertItem(FindFileData.cFileName, hParent, TVI_LAST);
				// 递归填充子目录和文件
                PopulateTree(hChild, subPath);
            }
        } else {// 是文件则直接添加到树中
			// 添加文件项到树中
            m_treeCtrl.InsertItem(FindFileData.cFileName, hParent, TVI_LAST);
        }
    } while (FindNextFile(hFind, &FindFileData));// 查找下一个匹配项，直到结束
	// 关闭查找句柄
    FindClose(hFind);
}

void CDecDlg::OnCancelMode() 
{
	CDialog::OnCancelMode();
	
	// TODO: Add your message handler code here
	
}
