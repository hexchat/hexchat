:: run this from a command prompt
@echo off

SET PACKAGE_NAME=libxml2-2.9.0

copy win32\vc10\Release\runsuite.exe .
set PATH=%PATH%;..\build\Win32\bin;win32\vc10\Release
runsuite.exe
del runsuite.exe
set LIBXML_SRC=%cd%
set LIBXML_DEST=%cd%-x86
echo.Press return when ready to install!
pause

rmdir /q /s %LIBXML_DEST%
mkdir %LIBXML_DEST%
mkdir %LIBXML_DEST%\bin
mkdir %LIBXML_DEST%\include
mkdir %LIBXML_DEST%\include\libxml
mkdir %LIBXML_DEST%\lib
mkdir %LIBXML_DEST%\share
mkdir %LIBXML_DEST%\share\doc
mkdir %LIBXML_DEST%\share\doc\libxml2
copy win32\vc10\Release\libxml2.dll %LIBXML_DEST%\bin
copy win32\vc10\Release\runsuite.exe %LIBXML_DEST%\bin
copy win32\vc10\Release\libxml2.lib %LIBXML_DEST%\lib
copy include\win32config.h %LIBXML_DEST%\include
copy include\wsockcompat.h %LIBXML_DEST%\include
xcopy /s include\libxml\*.h %LIBXML_DEST%\include\libxml\
copy COPYING %LIBXML_DEST%\share\doc\libxml2

cd %LIBXML_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %LIBXML_SRC%
rmdir /q /s %LIBXML_DEST%

echo.Finished!
pause
