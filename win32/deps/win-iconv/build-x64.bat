:: run this from a VS x64 command prompt
@echo off

SET PACKAGE_NAME=win-iconv-0.0.4

set ICONV_SRC=%cd%
set ICONV_DEST=%cd%-x64
del CMakeCache.txt
rmdir /q /s CMakeFiles
cmake -G "NMake Makefiles" -DCMAKE_INSTALL_PREFIX=%ICONV_DEST% -DCMAKE_BUILD_TYPE=Release
nmake clean
nmake
echo.Press return when ready to install!
pause

nmake install
mkdir %ICONV_DEST%\share
mkdir %ICONV_DEST%\share\doc
mkdir %ICONV_DEST%\share\doc\win-iconv
copy COPYING %ICONV_DEST%\share\doc\win-iconv
nmake clean

cd %ICONV_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x64.7z
7z a ..\%PACKAGE_NAME%-x64.7z *
cd %ICONV_SRC%
rmdir /q /s %ICONV_DEST%

echo.Finished!
pause
