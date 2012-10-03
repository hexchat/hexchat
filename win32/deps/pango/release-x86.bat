:: run this from a command prompt
@echo off

SET PACKAGE_NAME=pango-1.30.1

set PANGO_SRC=%cd%
set PANGO_DEST=%cd%-rel
echo.Press return when ready to install!
pause

mkdir %PANGO_DEST%\share
mkdir %PANGO_DEST%\share\doc
mkdir %PANGO_DEST%\share\doc\pango
copy COPYING %PANGO_DEST%\share\doc\pango

cd %PANGO_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %PANGO_SRC%
rmdir /q /s %PANGO_DEST%

echo.Finished!
pause
