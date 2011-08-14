@echo off
set PATH=%PATH%;%PROGRAMW6432%\7-Zip
cd ..\dep-x64
7z x *.zip
7z x *.7z
cd ..\dep-x86
7z x -y *.rpm
7z x *.cpio
del *.cpio
xcopy /q /s /i usr\i686-w64-mingw32\sys-root\mingw\* .
rmdir /q /s usr
7z x *.zip
7z x *.7z
pause
