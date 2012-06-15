@echo off
set PATH=%PATH%;%PROGRAMW6432%\7-Zip;c:\mozilla-build\gendef

cd Win32
::obs 7z x -y *.rpm
::obs 7z x *.cpio
::obs del *.cpio
::obs xcopy /q /s /i usr\i686-w64-mingw32\sys-root\mingw\* .
::obs rmdir /q /s usr
::obs set OPATH=%PATH%
::obs set PATH=%PATH%;c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;c:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE
::obs cd lib
::obs for %%A in (..\bin\*.dll) do (
::obs 	gendef %%A
::obs 	lib /nologo /machine:x86 /def:%%~nA.def
::obs )
::obs cd ..
::obs set PATH=%OPATH%
7z x -y *.zip
7z x -y *.7z
::obs copy /y ..\build\glibconfig-x86.h lib\glib-2.0\include\glibconfig.h
cd share\locale
del /q /s atk10.mo
del /q /s gettext-tools.mo
del /q /s gettext-runtime.mo
pause
