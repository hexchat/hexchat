@echo off
call build-x86.bat
call build-x64.bat
set PATH=c:\Program Files (x86)\Inno Setup 5
compil32 /cc xchat-wdk.iss
pause
