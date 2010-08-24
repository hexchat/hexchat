@echo off
set GTK_BIN=c:\mozilla-build\build\xchat-dev32\bin
set SSL_BIN=c:\mozilla-build\build\openssl-wdk-1.0.0a-x86\bin
set LUA_BIN=c:\mozilla-build\build\lua-wdk-5.1.4-2-x86\bin
set XCHAT_DEST=c:\mozilla-build\build\xchat-wdk-uni
rmdir /q /s %XCHAT_DEST%
mkdir %XCHAT_DEST%
xcopy /q ..\src\fe-gtk\xchat.exe %XCHAT_DEST%
xcopy /q %GTK_BIN%\libgtk-win32-2.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libgdk_pixbuf-2.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libgio-2.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libglib-2.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libgmodule-2.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libgobject-2.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libgthread-2.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libpng14-14.dll %XCHAT_DEST%
::xcopy /q %GTK_BIN%\zlib1.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libgdk-win32-2.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libcairo-2.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libfontconfig-1.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libexpat-1.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\freetype6.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libpango-1.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libpangocairo-1.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libpangoft2-1.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libpangowin32-1.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\libatk-1.0-0.dll %XCHAT_DEST%
xcopy /q %GTK_BIN%\intl.dll %XCHAT_DEST%
::xcopy /q /s /i %GTK_BIN%\..\lib\gtk-2.0 %XCHAT_DEST%\lib\gtk-2.0
::rmdir /q /s %XCHAT_DEST%\lib\gtk-2.0\include
xcopy /q /s /i %GTK_BIN%\..\lib\gtk-2.0\2.10.0\engines\libwimp.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines\
xcopy /q /s /i etc %XCHAT_DEST%\etc
xcopy /q ..\COPYING %XCHAT_DEST%
xcopy /q %SSL_BIN%\..\LICENSE.OPENSSL %XCHAT_DEST%
xcopy /q %SSL_BIN%\..\LICENSE.ZLIB %XCHAT_DEST%
xcopy /q %SSL_BIN%\libeay32.dll %XCHAT_DEST%
xcopy /q %SSL_BIN%\ssleay32.dll %XCHAT_DEST%
xcopy /q %SSL_BIN%\zlib1.dll %XCHAT_DEST%
xcopy /q /s /i ..\plugins\ewc\xcewc.dll %XCHAT_DEST%\plugins\
xcopy /q /s /i ..\plugins\lua\xclua.dll %XCHAT_DEST%\plugins\
xcopy /q /s /i ..\plugins\perl\xcperl.dll %XCHAT_DEST%\plugins\
xcopy /q /s /i ..\plugins\python\xcpython.dll %XCHAT_DEST%\plugins\
xcopy /q /s /i ..\plugins\tcl\xctcl.dll %XCHAT_DEST%\plugins\
xcopy /q /s /i ..\plugins\xdcc\xcxdcc.dll %XCHAT_DEST%\plugins\
xcopy /q %LUA_BIN%\lua51.dll %XCHAT_DEST%
xcopy /q /s /i ..\po\locale %XCHAT_DEST%\locale
xcopy /q /s /i %GTK_BIN%\..\share\locale %XCHAT_DEST%\share\locale
echo 2> portable-mode
move portable-mode %XCHAT_DEST%
