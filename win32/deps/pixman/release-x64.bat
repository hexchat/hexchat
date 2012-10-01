:: run this from a command prompt
@echo off

SET PACKAGE_NAME=pixman-0.26.2

set PIXMAN_SRC=%cd%
set PIXMAN_DEST=%cd%-rel
echo.Press return when ready to install!
pause

copy COPYING %PIXMAN_DEST%\LICENSE.PIXMAN

cd %PIXMAN_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x64.7z
7z a ..\%PACKAGE_NAME%-x64.7z *
cd %PIXMAN_SRC%
rmdir /q /s %PIXMAN_DEST%

echo.Finished!
pause
