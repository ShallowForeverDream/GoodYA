@echo off
setlocal

set "VC6ROOT=D:\0tools\VC6\Microsoft Visual Studio"
set "VCBIN=%VC6ROOT%\VC98\Bin"
set "MSDEVBIN=%VC6ROOT%\Common\MSDev98\Bin"
set "VCINC=%VC6ROOT%\VC98\INCLUDE"
set "MFCINC=%VC6ROOT%\VC98\MFC\INCLUDE"
set "ATLINC=%VC6ROOT%\VC98\ATL\INCLUDE"
set "VCLIB=%VC6ROOT%\VC98\LIB"
set "MFCLIB=%VC6ROOT%\VC98\MFC\LIB"

if not exist "%VCBIN%\CL.EXE" (
    echo [ERROR] CL.EXE not found: "%VCBIN%\CL.EXE"
    exit /b 1
)
if not exist "%MSDEVBIN%\RC.EXE" (
    echo [ERROR] RC.EXE not found: "%MSDEVBIN%\RC.EXE"
    exit /b 1
)

set "PATH=%VCBIN%;%MSDEVBIN%;%PATH%"
set "INCLUDE=%ATLINC%;%VCINC%;%MFCINC%"
set "LIB=%VCLIB%;%MFCLIB%"

if not exist "Debug" mkdir "Debug"
if exist "Debug\build_vc6.pdb" del /f /q "Debug\build_vc6.pdb" >nul 2>nul
if exist "Debug\build_vc6.idb" del /f /q "Debug\build_vc6.idb" >nul 2>nul
if exist "Debug\GoodYa.pch" del /f /q "Debug\GoodYa.pch" >nul 2>nul

set "CPPFLAGS=/nologo /MDd /W3 /GX /ZI /Od /D WIN32 /D _DEBUG /D _WINDOWS /D _AFXDLL /D _MBCS /FD /GZ"

echo [1/4] Compile PCH (StdAfx.cpp)...
cl %CPPFLAGS% /Yc"stdafx.h" /Fp"Debug\GoodYa.pch" /Fd"Debug\build_vc6.pdb" /Fo"Debug\StdAfx.obj" /c StdAfx.cpp
if errorlevel 1 exit /b 1

echo [2/4] Compile C++ sources...
for %%F in (ComDlg.cpp DecDlg.cpp GoodYa.cpp GoodYaDoc.cpp GoodYaView.cpp Huffman.cpp MainFrm.cpp PassDlg.cpp) do (
    echo    %%F
    cl %CPPFLAGS% /Yu"stdafx.h" /Fp"Debug\GoodYa.pch" /Fd"Debug\build_vc6.pdb" /Fo"Debug\%%~nF.obj" /c "%%F"
    if errorlevel 1 exit /b 1
)

echo [3/4] Compile resources...
rc /l 0x804 /d "_DEBUG" /d "_AFXDLL" /fo "Debug\GoodYa.res" GoodYa.rc
if errorlevel 1 exit /b 1

echo [4/4] Link...
link /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /pdb:"Debug\GoodYa.pdb" /out:"Debug\GoodYa.exe" "Debug\ComDlg.obj" "Debug\DecDlg.obj" "Debug\GoodYa.obj" "Debug\GoodYaDoc.obj" "Debug\GoodYaView.obj" "Debug\Huffman.obj" "Debug\MainFrm.obj" "Debug\PassDlg.obj" "Debug\StdAfx.obj" "Debug\GoodYa.res"
if errorlevel 1 exit /b 1

echo Build succeeded: Debug\GoodYa.exe
exit /b 0
