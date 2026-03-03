@echo off
setlocal

set "VC6ROOT=D:\Microsoft Visual Studio"
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
set "BUILD_TAG=%RANDOM%%RANDOM%%RANDOM%"
set "BUILD_PDB=Debug\build_vc6_%BUILD_TAG%.pdb"
set "BUILD_IDB=Debug\build_vc6_%BUILD_TAG%.idb"

if not exist "Debug" mkdir "Debug"
if exist "Debug\build_vc6.pdb" del /f /q "Debug\build_vc6.pdb" >nul 2>nul
if exist "Debug\build_vc6.idb" del /f /q "Debug\build_vc6.idb" >nul 2>nul
if exist "Debug\GoodYa.pch" del /f /q "Debug\GoodYa.pch" >nul 2>nul

set "CPPFLAGS=/nologo /MDd /W3 /GX /ZI /Od /D WIN32 /D _DEBUG /D _WINDOWS /D _AFXDLL /D _MBCS /FD /GZ"

echo [1/4] Compile PCH (StdAfx.cpp)...
call :compile_with_retry StdAfx.cpp "Debug\StdAfx.obj" Yc
if errorlevel 1 exit /b 1

echo [2/4] Compile C++ sources...
for %%F in (ComDlg.cpp DecDlg.cpp GoodYa.cpp GoodYaDoc.cpp GoodYaView.cpp Huffman.cpp MainFrm.cpp PassDlg.cpp) do (
    echo    %%F
    call :compile_with_retry "%%F" "Debug\%%~nF.obj" Yu
    if errorlevel 1 exit /b 1
)

echo [3/4] Compile resources...
rc /l 0x804 /d "_DEBUG" /d "_AFXDLL" /fo "Debug\GoodYa.res" GoodYa.rc
if errorlevel 1 exit /b 1

echo [4/4] Link...
taskkill /im GoodYa.exe /f >nul 2>nul
call :link_with_retry
if errorlevel 1 exit /b 1

echo Build succeeded: Debug\GoodYa.exe
exit /b 0

:compile_with_retry
set "SRC=%~1"
set "OBJ=%~2"
set "PCHMODE=%~3"
set "TRY_COUNT=0"

:compile_retry_loop
if /I "%PCHMODE%"=="Yc" (
    cl %CPPFLAGS% /Yc"stdafx.h" /Fp"Debug\GoodYa.pch" /Fd"%BUILD_PDB%" /Fo"%OBJ%" /c "%SRC%"
) else (
    cl %CPPFLAGS% /Yu"stdafx.h" /Fp"Debug\GoodYa.pch" /Fd"%BUILD_PDB%" /Fo"%OBJ%" /c "%SRC%"
)
if not errorlevel 1 exit /b 0

if "%TRY_COUNT%"=="0" (
    set "TRY_COUNT=1"
    echo [WARN] Compile failed for %SRC%. Retry once...
    if exist "%BUILD_PDB%" del /f /q "%BUILD_PDB%" >nul 2>nul
    if exist "%BUILD_IDB%" del /f /q "%BUILD_IDB%" >nul 2>nul
    ping 127.0.0.1 -n 2 >nul
    goto :compile_retry_loop
)

exit /b 1

:link_with_retry
set "LINK_TRY=0"

:link_retry_loop
link /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /pdb:"Debug\GoodYa.pdb" /out:"Debug\GoodYa.exe" "Debug\ComDlg.obj" "Debug\DecDlg.obj" "Debug\GoodYa.obj" "Debug\GoodYaDoc.obj" "Debug\GoodYaView.obj" "Debug\Huffman.obj" "Debug\MainFrm.obj" "Debug\PassDlg.obj" "Debug\StdAfx.obj" "Debug\GoodYa.res"
if not errorlevel 1 exit /b 0

if "%LINK_TRY%"=="0" (
    set "LINK_TRY=1"
    echo [WARN] Link failed. Retry once...
    taskkill /im GoodYa.exe /f >nul 2>nul
    ping 127.0.0.1 -n 2 >nul
    goto :link_retry_loop
)

exit /b 1
