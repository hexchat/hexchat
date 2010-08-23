@echo off
cd ..\src
echo X64 = YES > makeinc.mak
type makeinc.skel >> makeinc.mak
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\api\crt\stl70;c:\mozilla-build\build\xchat-dev64\include
set LIB=c:\WinDDK\7600.16385.1\lib\wnet\amd64;c:\WinDDK\7600.16385.1\lib\Crt\amd64
set OPATH=%PATH%
set PATH=c:\WinDDK\7600.16385.1\bin\x86\amd64;c:\WinDDK\7600.16385.1\bin\x86;c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\x64;c:\mozilla-build\build\xchat-dev64\bin;c:\mozilla-build\perl-5.12-x64\bin
nmake -f makefile.mak clean
nmake -f makefile.mak
cd ..\plugins\ewc
nmake -f makefile.mak clean
nmake -f makefile.mak
::cd ..\dns
::nmake -f makefile.mak clean
::nmake -f makefile.mak
cd ..\lua
nmake -f makefile.mak clean
nmake -f makefile.mak
cd ..\perl
nmake -f makefile.mak clean
nmake -f makefile.mak
cd ..\python
nmake -f makefile.mak clean
nmake -f makefile.mak
cd ..\tcl
nmake -f makefile.mak clean
nmake -f makefile.mak
cd ..\xdcc
nmake -f makefile.mak clean
nmake -f makefile.mak
::cd ..\xtray
::nmake -f makefile.mak clean
::nmake -f makefile.mak
cd ..\..\build
set PATH=%PATH%;c:\mozilla-build\build\xchat-dev32\bin
call compile-po-files.bat
cd ..\build
set PATH=%OPATH%
call release-x64.bat
set PATH=c:\Program Files (x86)\Inno Setup 5
compil32 /cc xchat-wdk-x64.iss
set PATH=%OPATH%
pause
