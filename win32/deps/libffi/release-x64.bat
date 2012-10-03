:: run this from a command prompt
@echo off

SET PACKAGE_NAME=libffi-3.0.11

set LIBFFI_SRC=%cd%
set LIBFFI_DEST=%cd%-x64
echo.Press return when ready to install!
pause

rmdir /q /s %LIBFFI_DEST%
mkdir %LIBFFI_DEST%
mkdir %LIBFFI_DEST%\bin
mkdir %LIBFFI_DEST%\include
mkdir %LIBFFI_DEST%\lib
mkdir %LIBFFI_DEST%\share
mkdir %LIBFFI_DEST%\share\doc
mkdir %LIBFFI_DEST%\share\doc\libffi
set OUTDIR=x86_64-w64-mingw32
copy %OUTDIR%\include\ffi.h %LIBFFI_DEST%\include
copy %OUTDIR%\include\ffitarget.h %LIBFFI_DEST%\include
copy %OUTDIR%\.libs\libffi_convenience.lib %LIBFFI_DEST%\lib\libffi.lib
copy %OUTDIR%\.libs\libffi-6.dll %LIBFFI_DEST%\bin
copy LICENSE %LIBFFI_DEST%\share\doc\libffi\COPYING

cd %LIBFFI_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x64.7z
7z a ..\%PACKAGE_NAME%-x64.7z *
cd %LIBFFI_SRC%
rmdir /q /s %LIBFFI_DEST%
rmdir /q /s %OUTDIR%

echo.Finished!
pause
