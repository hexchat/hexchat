:: run this from a command prompt
@echo off

SET PACKAGE_NAME=atk-2.5.91

set ATK_SRC=%cd%
set ATK_DEST=%cd%-rel
echo.Press return when ready to install!
pause

copy COPYING %ATK_DEST%\LICENSE.ATK

cd %ATK_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x64.7z
7z a ..\%PACKAGE_NAME%-x64.7z *
cd %ATK_SRC%
rmdir /q /s %ATK_DEST%

echo.Finished!
pause
