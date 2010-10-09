@echo off
set GTK_BIN=c:\mozilla-build\build\xchat-dev32\bin
set SSL_BIN=c:\mozilla-build\build\openssl-wdk-1.0.0a-x86\bin
set LUA_BIN=c:\mozilla-build\build\lua-wdk-5.1.4-2-x86\bin
set ENCHANT_BIN=c:\mozilla-build\build\enchant-wdk-1.6.0-x86\bin
set XCHAT_DEST=c:\mozilla-build\build\xchat-wdk-uni
rmdir /q /s %XCHAT_DEST%
mkdir %XCHAT_DEST%
echo 2> portable-mode
move portable-mode %XCHAT_DEST%
copy ..\src\fe-gtk\xchat.exe %XCHAT_DEST%
copy %GTK_BIN%\libgtk-win32-2.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libgdk_pixbuf-2.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libgio-2.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libglib-2.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libgmodule-2.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libgobject-2.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libgthread-2.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libpng14-14.dll %XCHAT_DEST%
copy %GTK_BIN%\libgdk-win32-2.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libcairo-2.dll %XCHAT_DEST%
copy %GTK_BIN%\libfontconfig-1.dll %XCHAT_DEST%
copy %GTK_BIN%\libexpat-1.dll %XCHAT_DEST%
copy %GTK_BIN%\freetype6.dll %XCHAT_DEST%
copy %GTK_BIN%\libpango-1.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libpangocairo-1.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libpangoft2-1.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libpangowin32-1.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\libatk-1.0-0.dll %XCHAT_DEST%
copy %GTK_BIN%\intl.dll %XCHAT_DEST%
xcopy /q /s /i %GTK_BIN%\..\lib\gtk-2.0\2.10.0\engines\libpixmap.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines\
copy %GTK_BIN%\..\lib\gtk-2.0\2.10.0\engines\libwimp.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines
xcopy /q /s /i %GTK_BIN%\..\lib\gtk-2.0\modules\libgail.dll %XCHAT_DEST%\lib\gtk-2.0\modules\
xcopy /q /s /i etc %XCHAT_DEST%\etc
copy ..\COPYING %XCHAT_DEST%
copy %SSL_BIN%\..\LICENSE.OPENSSL %XCHAT_DEST%
copy %SSL_BIN%\..\LICENSE.ZLIB %XCHAT_DEST%
copy %GTK_BIN%\..\share\doc\glib-2.26.0\COPYING %XCHAT_DEST%\LICENSE.GTK
copy %GTK_BIN%\..\share\doc\cairo_1.10.0-1_win32\COPYING-LGPL-2.1 %XCHAT_DEST%\LICENSE.CAIRO
copy %LUA_BIN%\..\COPYRIGHT %XCHAT_DEST%\LICENSE.LUA
copy %ENCHANT_BIN%\..\COPYING %XCHAT_DEST%\LICENSE.ENCHANT
copy %SSL_BIN%\libeay32.dll %XCHAT_DEST%
copy %SSL_BIN%\ssleay32.dll %XCHAT_DEST%
copy %SSL_BIN%\zlib1.dll %XCHAT_DEST%
copy %SSL_BIN%\cert.pem %XCHAT_DEST%
copy %ENCHANT_BIN%\libenchant.dll %XCHAT_DEST%
xcopy /q /s /i %ENCHANT_BIN%\..\lib\enchant\libenchant_myspell.dll %XCHAT_DEST%\lib\enchant\
::xcopy /q /s /i ..\plugins\ewc\xcewc.dll %XCHAT_DEST%\plugins\
xcopy /q /s /i ..\plugins\lua\xclua.dll %XCHAT_DEST%\plugins\
copy ..\plugins\perl\xcperl-510.dll %XCHAT_DEST%\plugins
copy ..\plugins\perl\xcperl-512.dll %XCHAT_DEST%\plugins
copy ..\plugins\python\xcpython.dll %XCHAT_DEST%\plugins
copy ..\plugins\tcl\xctcl.dll %XCHAT_DEST%\plugins
copy ..\plugins\upd\xcupd.dll %XCHAT_DEST%\plugins
copy ..\plugins\xdcc\xcxdcc.dll %XCHAT_DEST%\plugins
copy ..\plugins\xtray\xtray.dll %XCHAT_DEST%\plugins
copy ..\plugins\winamp\xcwinamp.dll %XCHAT_DEST%\plugins
copy %LUA_BIN%\lua51.dll %XCHAT_DEST%
xcopy /q /s /i ..\po\locale %XCHAT_DEST%\locale
xcopy /q /s /i %GTK_BIN%\..\share\locale %XCHAT_DEST%\share\locale
xcopy /q /s /i ..\..\myspell %XCHAT_DEST%\share\myspell
