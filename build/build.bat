@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
call build-x86.bat
call build-x64.bat
::call compress.bat
set PATH=%PROGRAMFILES(X86)%\Inno Setup 5
compil32 /cc xchat-wdk.iss
pause
