@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\api\crt\stl70;%WDK_ROOT%\inc\mfc42;%WDK_ROOT%\inc\ddk;%WDK_ROOT%\inc\api\dao360
set LIB=%WDK_ROOT%\lib\wxp\i386;%WDK_ROOT%\lib\Crt\i386;%WDK_ROOT%\lib\Mfc\i386;%WDK_ROOT%\lib\ATL\i386
set OPATH=%PATH%
set DEV_32=%cd%\..\dep-x86
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin;%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\Common7\IDE;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin;%DEV_32%\bin
set PERL_512=c:\mozilla-build\perl-5.12-x86\perl
set PERL_514=c:\mozilla-build\perl-5.14-x86
echo [Setup] > xchat-wdk.iss
echo WizardImageFile=%cd%\bitmaps\wizardimage.bmp >> xchat-wdk.iss
echo WizardSmallImageFile=%cd%\bitmaps\wizardsmallimage.bmp >> xchat-wdk.iss
del version.exe
cl /nologo version.c
version -a >> xchat-wdk.iss
version -v >> xchat-wdk.iss
version -i >> xchat-wdk.iss
version -o >> xchat-wdk.iss
cd ..
build\version -r > resource.h
echo SetupIconFile=%cd%\xchat.ico >> build\xchat-wdk.iss
type build\xchat-wdk.skel.iss >> build\xchat-wdk.iss
cd src
echo DEV = %DEV_32% > makeinc.mak
type makeinc.skel.mak >> makeinc.mak
nmake /nologo /f makefile.mak clean
cd pixmaps
nmake /nologo /f makefile.mak
cd ..
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
call release-x86.bat
