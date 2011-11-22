@echo off
set PATH=%PROGRAMW6432%\7-Zip
cd ..\dep-x64
7z x *.zip
7z x *.7z
cd ..\dep-x86
7z x *.zip
7z x *.7z
pause
