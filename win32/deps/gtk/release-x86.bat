:: run this from a command prompt
@echo off

SET PACKAGE_NAME=gtk-2.24.13

set GTK_SRC=%cd%
set GTK_DEST=%cd%-rel
set MSGFMT_PATH=..\..\msgfmt
:: we'll go 1 level deeper
set PATH=%PATH%;..\%MSGFMT_PATH%
rmdir /q /s "%GTK_DEST%\share\locale"
mkdir "%GTK_DEST%\share\locale"
cd po
for %%A in (*.po) do (
mkdir "%GTK_DEST%\share\locale\%%~nA\LC_MESSAGES"
"msgfmt" -co "%GTK_DEST%\share\locale\%%~nA\LC_MESSAGES\gtk20.mo" %%A
)
mkdir "%GTK_DEST%\share\doc
mkdir "%GTK_DEST%\share\doc\gtk
cd ..
echo.Press return when ready to install!
pause

copy COPYING %GTK_DEST%\share\doc\gtk

cd %GTK_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %GTK_SRC%
rmdir /q /s %GTK_DEST%

echo.Finished!
pause
