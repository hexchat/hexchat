@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\api\crt\stl70;%WDK_ROOT%\inc\mfc42;%WDK_ROOT%\inc\ddk;%WDK_ROOT%\inc\api\dao360
call build-x86.bat
call build-x64.bat
::call compress.bat
set PATH=%PROGRAMFILES(X86)%\Inno Setup 5
compil32 /cc xchat-wdk.iss
pause
