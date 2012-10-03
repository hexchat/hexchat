:: run this from a VS x86 command prompt
@echo off

SET PACKAGE_NAME=enchant-1.6.0

set ENCHANT_SRC=%cd%
set ENCHANT_DEST=%cd%-x86
cd src
nmake -f makefile.mak clean
nmake -f makefile.mak DLL=1 MFLAGS=-MD GLIBDIR=..\..\build\Win32\include\glib-2.0
cd ..
echo.Press return when ready to install!
pause

set RELEASE_DIR=bin\release
rmdir /q /s %ENCHANT_DEST%
mkdir %ENCHANT_DEST%
mkdir %ENCHANT_DEST%\bin
mkdir %ENCHANT_DEST%\include
mkdir %ENCHANT_DEST%\include\enchant
mkdir %ENCHANT_DEST%\lib
mkdir %ENCHANT_DEST%\lib\enchant
mkdir %ENCHANT_DEST%\share
mkdir %ENCHANT_DEST%\share\doc
mkdir %ENCHANT_DEST%\share\doc\enchant
copy %RELEASE_DIR%\enchant.exe %ENCHANT_DEST%\bin
copy %RELEASE_DIR%\enchant-lsmod.exe %ENCHANT_DEST%\bin
copy %RELEASE_DIR%\libenchant.dll %ENCHANT_DEST%\bin
copy src\enchant.h %ENCHANT_DEST%\include\enchant
copy "src\enchant++.h" %ENCHANT_DEST%\include\enchant
copy src\enchant-provider.h %ENCHANT_DEST%\include\enchant
copy %RELEASE_DIR%\libenchant_ispell.dll %ENCHANT_DEST%\lib\enchant
copy %RELEASE_DIR%\libenchant_ispell.lib %ENCHANT_DEST%\lib\enchant
copy %RELEASE_DIR%\libenchant_myspell.dll %ENCHANT_DEST%\lib\enchant
copy %RELEASE_DIR%\libenchant_myspell.lib %ENCHANT_DEST%\lib\enchant
copy %RELEASE_DIR%\libenchant.lib %ENCHANT_DEST%\lib
copy COPYING.LIB %ENCHANT_DEST%\share\doc\enchant\COPYING
cd src
nmake -f makefile.mak clean

cd %ENCHANT_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %ENCHANT_SRC%
rmdir /q /s %ENCHANT_DEST%

echo.Finished!
pause
