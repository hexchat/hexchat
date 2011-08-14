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
set OPATH=%PATH%
set PATH=%PATH%;c:\mozilla-build\python-2.7-x86
python ..\build\convert-lib-x86.py
set PATH=%OPATH%
7z x *.7z
copy /y ..\build\glibconfig-x86.h lib\glib-2.0\include\glibconfig.h
pause
