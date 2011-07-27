@echo off
set PERL_512=c:\mozilla-build\perl-5.12-x64\perl
set PERL_514=c:\mozilla-build\perl-5.14-x64
cd ..
set DEV_64=%cd%\dep-x64
cd src
echo X64 = YES > makeinc.mak
echo DEV = %DEV_64% >> makeinc.mak
type makeinc.skel.mak >> makeinc.mak
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\api\crt\stl70
set LIB=%WDK_ROOT%\lib\wnet\amd64;%WDK_ROOT%\lib\Crt\amd64
set OPATH=%PATH%
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin\amd64;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin\x64;%DEV_64%\bin
cd common
nmake /nologo /f makefile.mak clean
cd ..\fe-gtk
nmake /nologo /f makefile.mak clean
cd ..\fe-text
nmake /nologo /f makefile.mak clean
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
set PATH=%OPATH%
call release-x64.bat
