@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\api\crt\stl70
set LIB=%WDK_ROOT%\lib\wnet\amd64;%WDK_ROOT%\lib\Crt\amd64
set OPATH=%PATH%
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin\amd64;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin\x64
nmake -f makefile.mak clean
nmake -f makefile.mak X64=1 DLL=1 MFLAGS=-MD GLIBDIR=..\glib-x64\include\glib-2.0
echo.Press return when ready to install!
pause
set PATH=%OPATH%
call release-x64.bat
pause
