:: run this from a command prompt
@echo off

SET PACKAGE_NAME=expat-2.1.0

set EXPAT_SRC=%cd%
set EXPAT_DEST=%cd%-x64
echo.Press return when ready to install!
pause

rmdir /q /s %EXPAT_DEST%
mkdir %EXPAT_DEST%
mkdir %EXPAT_DEST%\bin
mkdir %EXPAT_DEST%\include
mkdir %EXPAT_DEST%\lib
mkdir %EXPAT_DEST%\share
mkdir %EXPAT_DEST%\share\doc
mkdir %EXPAT_DEST%\share\doc\expat
copy win32\bin\Release\elements.exe %EXPAT_DEST%\bin
copy win32\bin\Release\libexpat.dll %EXPAT_DEST%\bin
copy win32\bin\Release\libexpatw.dll %EXPAT_DEST%\bin
copy win32\bin\Release\outline.exe %EXPAT_DEST%\bin
copy win32\bin\Release\xmlwf.exe %EXPAT_DEST%\bin
copy lib\expat.h %EXPAT_DEST%\include
copy lib\expat_external.h %EXPAT_DEST%\include
copy win32\bin\Release\libexpat.exp %EXPAT_DEST%\lib
copy win32\bin\Release\libexpat.lib %EXPAT_DEST%\lib
copy win32\bin\Release\libexpatMT.lib %EXPAT_DEST%\lib
copy win32\bin\Release\libexpatw.exp %EXPAT_DEST%\lib
copy win32\bin\Release\libexpatw.lib %EXPAT_DEST%\lib
copy win32\bin\Release\libexpatwMT.lib %EXPAT_DEST%\lib
copy COPYING %EXPAT_DEST%\share\doc\expat

cd %EXPAT_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x64.7z
7z a ..\%PACKAGE_NAME%-x64.7z *
cd %EXPAT_SRC%
rmdir /q /s %EXPAT_DEST%

echo.Finished!
pause
