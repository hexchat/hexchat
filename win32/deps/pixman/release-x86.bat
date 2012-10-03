:: run this from a command prompt
@echo off

SET PACKAGE_NAME=pixman-0.26.2

set PIXMAN_SRC=%cd%
set PIXMAN_DEST=%cd%-rel
echo.Press return when ready to install!
pause

mkdir %PIXMAN_DEST%\share
mkdir %PIXMAN_DEST%\share\doc
mkdir %PIXMAN_DEST%\share\doc\pixman
copy COPYING %PIXMAN_DEST%\share\doc\pixman

cd %PIXMAN_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %PIXMAN_SRC%
rmdir /q /s %PIXMAN_DEST%

echo.Finished!
pause
