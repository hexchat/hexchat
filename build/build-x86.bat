@echo off
cd ..\src
type makeinc.skel > makeinc.mak
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\api\crt\stl70;c:\mozilla-build\build\xchat-dev32\include
set LIB=c:\WinDDK\7600.16385.1\lib\wxp\i386;c:\WinDDK\7600.16385.1\lib\Crt\i386
set OPATH=%PATH%
set PATH=c:\WinDDK\7600.16385.1\bin\x86\x86;c:\WinDDK\7600.16385.1\bin\x86;c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin;c:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE;c:\mozilla-build\build\xchat-dev32\bin;c:\mozilla-build\mingw32\bin;c:\mozilla-build\perl-5.12-x86\bin
nmake /nologo /f makefile.mak clean
cd pixmaps
nmake /nologo /f makefile.mak
cd ..
nmake /nologo /f makefile.mak
cd ..\plugins
nmake /nologo /f makefile.mak clean
nmake /nologo /f makefile.mak
cd ..\build
call compile-po-files.bat
cd ..\build
set PATH=%OPATH%
call release-x86.bat
