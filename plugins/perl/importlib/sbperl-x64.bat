@echo off
set PATH=c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64
cd c:\mozilla-build\perl-5.12-x64\perl\lib\CORE
echo.Overwrite existing def file?
pause
dumpbin /exports ..\..\bin\perl512.dll > perl512.def
echo.Please adjust the resulting file manually, then hit return!
pause
lib /machine:x64 /def:perl512.def
pause
