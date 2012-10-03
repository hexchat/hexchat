:: run this from a command prompt
@echo off

SET PACKAGE_NAME=gdk-pixbuf-2.26.4

set GDKPIXBUF_SRC=%cd%
set GDKPIXBUF_DEST=%cd%-rel
echo.Press return when ready to install!
pause

mkdir %GDKPIXBUF_DEST%\share
mkdir %GDKPIXBUF_DEST%\share\doc
mkdir %GDKPIXBUF_DEST%\share\doc\gdk-pixbuf
copy COPYING %GDKPIXBUF_DEST%\share\doc\gdk-pixbuf

cd %GDKPIXBUF_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %GDKPIXBUF_SRC%
rmdir /q /s %GDKPIXBUF_DEST%

echo.Finished!
pause
