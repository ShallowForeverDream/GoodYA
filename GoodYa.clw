; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CPassDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "GoodYa.h"
LastPage=0

ClassCount=8
Class1=CGoodYaApp
Class2=CGoodYaDoc
Class3=CGoodYaView
Class4=CMainFrame

ResourceCount=5
Resource1=IDD_DIALOG1
Resource2=IDR_MAINFRAME
Resource3=IDD_ABOUTBOX
Class5=CAboutDlg
Class6=CComDlg
Class7=CDecDlg
Resource4=IDD_DIALOG2
Class8=CPassDlg
Resource5=IDD_PASSDLG

[CLS:CGoodYaApp]
Type=0
HeaderFile=GoodYa.h
ImplementationFile=GoodYa.cpp
Filter=N

[CLS:CGoodYaDoc]
Type=0
HeaderFile=GoodYaDoc.h
ImplementationFile=GoodYaDoc.cpp
Filter=N

[CLS:CGoodYaView]
Type=0
HeaderFile=GoodYaView.h
ImplementationFile=GoodYaView.cpp
Filter=C
BaseClass=CView
VirtualFilter=VWC
LastObject=CGoodYaView


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=CMainFrame
BaseClass=CFrameWnd
VirtualFilter=fWC




[CLS:CAboutDlg]
Type=0
HeaderFile=GoodYa.cpp
ImplementationFile=GoodYa.cpp
Filter=D
LastObject=CAboutDlg
BaseClass=CDialog
VirtualFilter=dWC

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_SAVE_AS
Command5=ID_FILE_PRINT
Command6=ID_FILE_PRINT_PREVIEW
Command7=ID_FILE_PRINT_SETUP
Command8=ID_FILE_MRU_FILE1
Command9=ID_APP_EXIT
Command10=ID_EDIT_UNDO
Command11=ID_EDIT_CUT
Command12=ID_EDIT_COPY
Command13=ID_EDIT_PASTE
Command14=ID_VIEW_TOOLBAR
Command15=ID_VIEW_STATUS_BAR
Command16=ID_APP_ABOUT
Command17=IDM_COM
Command18=IDM_DEC
CommandCount=18

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command13=ID_NEXT_PANE
Command14=ID_PREV_PANE
CommandCount=14

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
Command8=ID_APP_ABOUT
Command9=IDM_COM
Command10=IDM_DEC
CommandCount=10

[DLG:IDD_DIALOG1]
Type=1
Class=CComDlg
ControlCount=17
Control1=IDC_STATIC,static,1342308352
Control2=IDC_OPEN,button,1342242816
Control3=IDC_EDIT1,edit,1350631552
Control4=IDC_STATIC,button,1342177287
Control5=IDC_RADIO1,button,1342308361
Control6=IDC_RADIO2,button,1342177289
Control7=IDC_STATIC,static,1342308352
Control8=IDC_COMBO1,combobox,1344339971
Control9=IDC_STATIC,static,1342308352
Control10=IDC_COMBO2,combobox,1344339971
Control11=IDC_SETPASS,button,1342242816
Control12=IDC_STATIC,button,1342177287
Control13=IDC_CHECK1,button,1342242819
Control14=IDC_CHECK2,button,1342242819
Control15=IDC_CHECK3,button,1342242819
Control16=IDC_OK,button,1342242816
Control17=IDC_CANCEL,button,1342242816

[DLG:IDD_DIALOG2]
Type=1
Class=CDecDlg
ControlCount=15
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT2,edit,1350631552
Control3=IDC_STATIC,button,1342177287
Control4=IDC_RADIO1,button,1342308361
Control5=IDC_RADIO2,button,1342177289
Control6=IDC_RADIO3,button,1342177289
Control7=IDC_STATIC,button,1342177287
Control8=IDC_RADIO4,button,1342308361
Control9=IDC_RADIO5,button,1342177289
Control10=IDC_RADIO6,button,1342177289
Control11=IDC_RADIO7,button,1342177289
Control12=IDC_TREE1,SysTreeView32,1350631424
Control13=IDC_BUTTON1,button,1342242816
Control14=IDC_BUTTON2,button,1342242816
Control15=IDC_BROWSE,button,1342242816

[CLS:CComDlg]
Type=0
HeaderFile=ComDlg.h
ImplementationFile=ComDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CComDlg
VirtualFilter=dWC

[CLS:CDecDlg]
Type=0
HeaderFile=DecDlg.h
ImplementationFile=DecDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CDecDlg
VirtualFilter=dWC

[DLG:IDD_PASSDLG]
Type=1
Class=CPassDlg
ControlCount=4
Control1=IDC_STATIC_PASS_PROMPT,static,1342308352
Control2=IDC_EDIT_PASS,edit,1350631584
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816

[CLS:CPassDlg]
Type=0
HeaderFile=PassDlg.h
ImplementationFile=PassDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CPassDlg

