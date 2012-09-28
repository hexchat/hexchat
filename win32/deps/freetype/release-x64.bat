:: run this from a command prompt
@echo off

SET PACKAGE_NAME=freetype-2.4.10

set FREETYPE_SRC=%cd%
set FREETYPE_DEST=%cd%-x64
echo.Press return when ready to install!
pause

rmdir /q /s %FREETYPE_DEST%
mkdir %FREETYPE_DEST%
mkdir %FREETYPE_DEST%\include
mkdir %FREETYPE_DEST%\lib
xcopy /s include %FREETYPE_DEST%\include\
copy builds\win32\vc2012\x64\Release\freetype2410.lib %FREETYPE_DEST%\lib\freetype.lib
copy docs\LICENSE.TXT %FREETYPE_DEST%\LICENSE.FREETYPE

cd %FREETYPE_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x64.7z
7z a ..\%PACKAGE_NAME%-x64.7z *
cd %FREETYPE_SRC%
rmdir /q /s %FREETYPE_DEST%

echo.Finished!
pause
