@echo off
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\api\crt\stl70;c:\mozilla-build\build\xchat-dev32\include;c:\mozilla-build\build\openssl-1.0.0a-wdk-x86\include
set LIB=c:\WinDDK\7600.16385.1\lib\wxp\i386;c:\WinDDK\7600.16385.1\lib\Crt\i386;c:\mozilla-build\build\openssl-1.0.0a-wdk-x86\lib
set OPATH=%PATH%
set PATH=c:\WinDDK\7600.16385.1\bin\x86\x86;c:\WinDDK\7600.16385.1\bin\x86;c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin;c:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE;c:\mozilla-build\build\xchat-dev32\bin;c:\mozilla-build\mingw\bin;c:\mozilla-build\perl-5.10-x86\bin
cd ..\src
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
call compile-po-files.bat
cd ..\build
set PATH=%OPATH%
call release-x86.bat
pause
