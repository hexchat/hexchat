@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\api\crt\stl70
set LIB=%WDK_ROOT%\lib\wxp\i386;%WDK_ROOT%\lib\Crt\i386
set OPATH=%PATH%
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin;%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\Common7\IDE
nmake -f makefile.mak clean
nmake -f makefile.mak DLL=1 MFLAGS=-MD GLIBDIR=..\glib-x86\include\glib-2.0
echo.Press return when ready to install!
pause
set PATH=%OPATH%
call release-x86.bat
pause
