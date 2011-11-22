@echo off
set PATH=%PROGRAMW6432%\7-Zip
cd ..\dep-x64
7z -y x *.zip
7z x *.7z
cd share\locale
del /q /s gettext-tools.mo
del /q /s gettext-runtime.mo
cd ..\..\..\dep-x86
7z -y x *.zip
7z x *.7z
cd share\locale
del /q /s gettext-tools.mo
del /q /s gettext-runtime.mo
pause
