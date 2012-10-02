:: run this from a command prompt
@echo off

SET PACKAGE_NAME=gtk-2.24.13

set GTK_SRC=%cd%
set GTK_DEST=%cd%-rel
echo.Press return when ready to install!
pause

copy COPYING %GTK_DEST%\LICENSE.GTK

cd %GTK_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %GTK_SRC%
rmdir /q /s %GTK_DEST%

echo.Finished!
pause
