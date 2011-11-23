@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\api\crt\stl70;%WDK_ROOT%\inc\mfc42;%WDK_ROOT%\inc\ddk;%WDK_ROOT%\inc\api\dao360
set LIB=%WDK_ROOT%\lib\wnet\amd64;%WDK_ROOT%\lib\Crt\amd64;%WDK_ROOT%\lib\Mfc\amd64;%WDK_ROOT%\lib\ATL\amd64
set OPATH=%PATH%
set DEV_64=%cd%\..\dep-x64
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin\amd64;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin\x64;%DEV_64%\bin
set PERL_512=c:\mozilla-build\perl-5.12-x64\perl
set PERL_514=c:\mozilla-build\perl-5.14-x64
echo [Setup] > xchat-wdk-x64.iss
echo WizardImageFile=%cd%\bitmaps\wizardimage.bmp >> xchat-wdk-x64.iss
echo WizardSmallImageFile=%cd%\bitmaps\wizardsmallimage.bmp >> xchat-wdk-x64.iss
del version.exe
cl /nologo version.c
version -a64 >> xchat-wdk-x64.iss
version -v >> xchat-wdk-x64.iss
version -i >> xchat-wdk-x64.iss
version -o64 >> xchat-wdk-x64.iss
cd ..
build\version -r > resource.h
echo SetupIconFile=%cd%\xchat.ico >> build\xchat-wdk-x64.iss
type build\xchat-wdk-x64.skel.iss >> build\xchat-wdk-x64.iss
cd src
echo DEV = %DEV_64% > makeinc.mak
echo X64 = YES >> makeinc.mak
type makeinc.skel.mak >> makeinc.mak
nmake /nologo /f makefile.mak clean
nmake /nologo /f makefile.mak
cd ..\plugins
nmake /nologo /f makefile.mak clean
nmake /nologo /f makefile.mak
cd perl
set OOPATH=%PATH%
set PATH=%OOPATH%;%PERL_512%\bin
nmake /nologo /s /f makefile-512.mak clean
nmake /nologo /s /f makefile-512.mak
set PATH=%OOPATH%;%PERL_514%\bin
nmake /nologo /s /f makefile-514.mak clean
nmake /nologo /s /f makefile-514.mak
cd ..\..\build
call compile-po-files.bat
set PATH=%OPATH%
call release-x64.bat
set PATH=%PROGRAMFILES(X86)%\Inno Setup 5
compil32 /cc xchat-wdk-x64.iss
pause
