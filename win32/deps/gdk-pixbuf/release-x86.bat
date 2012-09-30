:: run this from a command prompt
@echo off

SET PACKAGE_NAME=gdk-pixbuf-2.26.4

set GDKPIXBUF_SRC=%cd%
set GDKPIXBUF_DEST=%cd%-rel
echo.Press return when ready to install!
pause

copy COPYING %GDKPIXBUF_DEST%\LICENSE.GDKPIXBUF

cd %GDKPIXBUF_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %GDKPIXBUF_SRC%
rmdir /q /s %GDKPIXBUF_DEST%

echo.Finished!
pause
