@echo off
set GTK_BIN=c:\mozilla-build\build\xchat-dev64\bin
set SSL_BIN=c:\mozilla-build\build\openssl-wdk-1.0.0a-x64\bin
set LUA_BIN=c:\mozilla-build\build\lua-wdk-5.1.4-2-x64\bin
set XCHAT_DEST=c:\mozilla-build\build\xchat-wdk-uni
copy ..\src\fe-gtk\xchat.exe %XCHAT_DEST%\xchat.exe.x64
copy %GTK_BIN%\libgtk-win32-2.0-0.dll %XCHAT_DEST%\libgtk-win32-2.0-0.dll.x64
copy %GTK_BIN%\libgdk_pixbuf-2.0-0.dll %XCHAT_DEST%\libgdk_pixbuf-2.0-0.dll.x64
copy %GTK_BIN%\libgio-2.0-0.dll %XCHAT_DEST%\libgio-2.0-0.dll.x64
copy %GTK_BIN%\libglib-2.0-0.dll %XCHAT_DEST%\libglib-2.0-0.dll.x64
copy %GTK_BIN%\libgmodule-2.0-0.dll %XCHAT_DEST%\libgmodule-2.0-0.dll.x64
copy %GTK_BIN%\libgobject-2.0-0.dll %XCHAT_DEST%\libgobject-2.0-0.dll.x64
copy %GTK_BIN%\libgthread-2.0-0.dll %XCHAT_DEST%\libgthread-2.0-0.dll.x64
copy %GTK_BIN%\libpng14-14.dll %XCHAT_DEST%\libpng14-14.dll.x64
copy %GTK_BIN%\libgdk-win32-2.0-0.dll %XCHAT_DEST%\libgdk-win32-2.0-0.dll.x64
copy %GTK_BIN%\libcairo-2.dll %XCHAT_DEST%\libcairo-2.dll.x64
copy %GTK_BIN%\libfontconfig-1.dll %XCHAT_DEST%\libfontconfig-1.dll.x64
copy %GTK_BIN%\libexpat-1.dll %XCHAT_DEST%\libexpat-1.dll.x64
copy %GTK_BIN%\libfreetype-6.dll %XCHAT_DEST%\libfreetype-6.dll.x64
copy %GTK_BIN%\libpango-1.0-0.dll %XCHAT_DEST%\libpango-1.0-0.dll.x64
copy %GTK_BIN%\libpangocairo-1.0-0.dll %XCHAT_DEST%\libpangocairo-1.0-0.dll.x64
copy %GTK_BIN%\libpangoft2-1.0-0.dll %XCHAT_DEST%\libpangoft2-1.0-0.dll.x64
copy %GTK_BIN%\libpangowin32-1.0-0.dll %XCHAT_DEST%\libpangowin32-1.0-0.dll.x64
copy %GTK_BIN%\libatk-1.0-0.dll %XCHAT_DEST%\libatk-1.0-0.dll.x64
copy %GTK_BIN%\libintl-8.dll %XCHAT_DEST%\libintl-8.dll.x64
copy %GTK_BIN%\..\lib\gtk-2.0\2.10.0\engines\libpixmap.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines\libpixmap.dll.x64
copy %GTK_BIN%\..\lib\gtk-2.0\2.10.0\engines\libwimp.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines\libwimp.dll.x64
copy %GTK_BIN%\..\lib\gtk-2.0\modules\libgail.dll %XCHAT_DEST%\lib\gtk-2.0\modules\libgail.dll.x64
copy %SSL_BIN%\libeay32.dll %XCHAT_DEST%\libeay32.dll.x64
copy %SSL_BIN%\ssleay32.dll %XCHAT_DEST%\ssleay32.dll.x64
copy %SSL_BIN%\zlib1.dll %XCHAT_DEST%\zlib1.dll.x64
copy %GTK_BIN%\libenchant.dll %XCHAT_DEST%\libenchant.dll.x64
copy %GTK_BIN%\..\lib\enchant\libenchant_myspell.dll %XCHAT_DEST%\lib\enchant\libenchant_myspell.dll.x64
copy ..\plugins\ewc\xcewc.dll %XCHAT_DEST%\plugins\xcewc.dll.x64
copy ..\plugins\lua\xclua.dll %XCHAT_DEST%\plugins\xclua.dll.x64
copy ..\plugins\perl\xcperl-58.dll %XCHAT_DEST%\plugins\xcperl-58.dll.x64
copy ..\plugins\perl\xcperl-510.dll %XCHAT_DEST%\plugins\xcperl-510.dll.x64
copy ..\plugins\perl\xcperl-512.dll %XCHAT_DEST%\plugins\xcperl-512.dll.x64
copy ..\plugins\python\xcpython.dll %XCHAT_DEST%\plugins\xcpython.dll.x64
copy ..\plugins\tcl\xctcl.dll %XCHAT_DEST%\plugins\xctcl.dll.x64
copy ..\plugins\upd\xcupd.dll %XCHAT_DEST%\plugins\xcupd.dll.x64
copy ..\plugins\xdcc\xcxdcc.dll %XCHAT_DEST%\plugins\xcxdcc.dll.x64
copy ..\plugins\xtray\xtray.dll %XCHAT_DEST%\plugins\xtray.dll.x64
copy %LUA_BIN%\lua51.dll %XCHAT_DEST%\lua51.dll.x64
