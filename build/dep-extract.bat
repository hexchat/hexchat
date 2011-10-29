@echo off
set PATH=%PATH%;%PROGRAMW6432%\7-Zip;c:\mozilla-build\gendef

cd ..\dep-x64
::7z x -y *.rpm
::7z x *.cpio
::del *.cpio
::xcopy /q /s /i usr\x86_64-w64-mingw32\sys-root\mingw\* .
::rmdir /q /s usr
::set OPATH=%PATH%
::set PATH=%PATH%;c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64
::cd lib
::for %%A in (..\bin\*.dll) do (
::	gendef %%A
::	lib /nologo /machine:x64 /def:%%~nA.def
::)
::cd ..
::set PATH=%OPATH%
7z x *.7z
7z x *.zip
::copy /y ..\build\glibconfig-x64.h lib\glib-2.0\include\glibconfig.h

cd ..\dep-x86
::7z x -y *.rpm
::7z x *.cpio
::del *.cpio
::xcopy /q /s /i usr\i686-w64-mingw32\sys-root\mingw\* .
::rmdir /q /s usr
::set OPATH=%PATH%
::set PATH=%PATH%;c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;c:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE
::cd lib
::for %%A in (..\bin\*.dll) do (
::	gendef ..\bin\%%A
::	lib /nologo /machine:x86 /def:%%~nA.def
::)
::cd ..
::set PATH=%OPATH%
7z x *.7z
7z x *.zip
::copy /y ..\build\glibconfig-x86.h lib\glib-2.0\include\glibconfig.h

pause
