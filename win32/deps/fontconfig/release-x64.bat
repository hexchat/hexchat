:: run this from a command prompt
@echo off

SET PACKAGE_NAME=fontconfig-2.8.0

set FONTCONFIG_SRC=%cd%
set FONTCONFIG_DEST=%cd%-x64
echo.Press return when ready to install!
pause

rmdir /q /s %FONTCONFIG_DEST%
mkdir %FONTCONFIG_DEST%
mkdir %FONTCONFIG_DEST%\bin
mkdir %FONTCONFIG_DEST%\etc
mkdir %FONTCONFIG_DEST%\etc\fonts
mkdir %FONTCONFIG_DEST%\include
mkdir %FONTCONFIG_DEST%\include\fontconfig
mkdir %FONTCONFIG_DEST%\lib
mkdir %FONTCONFIG_DEST%\share
mkdir %FONTCONFIG_DEST%\share\doc
mkdir %FONTCONFIG_DEST%\share\doc\fontconfig
copy x64\Release\fontconfig.dll %FONTCONFIG_DEST%\bin
copy x64\Release\fc-cache.exe %FONTCONFIG_DEST%\bin
copy x64\Release\fc-cat.exe %FONTCONFIG_DEST%\bin
copy x64\Release\fc-list.exe %FONTCONFIG_DEST%\bin
copy x64\Release\fc-match.exe %FONTCONFIG_DEST%\bin
copy x64\Release\fc-query.exe %FONTCONFIG_DEST%\bin
copy x64\Release\fc-scan.exe %FONTCONFIG_DEST%\bin
copy fonts.conf %FONTCONFIG_DEST%\etc\fonts
copy fonts.dtd %FONTCONFIG_DEST%\etc\fonts
copy fontconfig\fcfreetype.h %FONTCONFIG_DEST%\include\fontconfig
copy fontconfig\fcprivate.h %FONTCONFIG_DEST%\include\fontconfig
copy fontconfig\fontconfig.h %FONTCONFIG_DEST%\include\fontconfig
copy x64\Release\fontconfig.lib %FONTCONFIG_DEST%\lib
copy COPYING %FONTCONFIG_DEST%\share\doc\fontconfig

cd %FONTCONFIG_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x64.7z
7z a ..\%PACKAGE_NAME%-x64.7z *
cd %FONTCONFIG_SRC%
rmdir /q /s %FONTCONFIG_DEST%

echo.Finished!
pause
