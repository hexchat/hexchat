:: run this from a command prompt
@echo off

SET PACKAGE_NAME=cairo-1.10.2

set CAIRO_SRC=%cd%
set CAIRO_DEST=%cd%-rel
echo.Press return when ready to install!
pause

mkdir %CAIRO_DEST%\share
mkdir %CAIRO_DEST%\share\doc
mkdir %CAIRO_DEST%\share\doc\cairo
copy COPYING %CAIRO_DEST%\share\doc\cairo

cd %CAIRO_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x64.7z
7z a ..\%PACKAGE_NAME%-x64.7z *
cd %CAIRO_SRC%
rmdir /q /s %CAIRO_DEST%

echo.Finished!
pause
