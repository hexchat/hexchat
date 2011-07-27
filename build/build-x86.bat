@echo off
set PERL_512=c:\mozilla-build\perl-5.12-x86\perl
set PERL_514=c:\mozilla-build\perl-5.14-x86
echo [Setup] > xchat-wdk.iss
echo WizardImageFile=%cd%\bitmaps\wizardimage.bmp >> xchat-wdk.iss
echo WizardSmallImageFile=%cd%\bitmaps\wizardsmallimage.bmp >> xchat-wdk.iss
cd ..
echo SetupIconFile=%cd%\xchat.ico >> build\xchat-wdk.iss
type build\xchat-wdk.skel.iss >> build\xchat-wdk.iss
set DEV_32=%cd%\dep-x86
cd src
echo DEV = %DEV_32% > makeinc.mak
type makeinc.skel.mak >> makeinc.mak
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\api\crt\stl70
set LIB=%WDK_ROOT%\lib\wxp\i386;%WDK_ROOT%\lib\Crt\i386
set OPATH=%PATH%
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin;%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\Common7\IDE;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin;%DEV_32%\bin
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
