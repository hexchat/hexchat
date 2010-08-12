@echo off
set GTK_BIN=c:\mozilla-build\build\xchat-dev32\bin
set SSL_BIN=c:\mozilla-build\build\openssl-1.0.0a-wdk-x86\bin
set XCHAT_DEST=c:\mozilla-build\build\xchat-wdk-x86
rmdir /Q /S %XCHAT_DEST%
mkdir %XCHAT_DEST%
xcopy ..\src\fe-gtk\xchat.exe %XCHAT_DEST%
xcopy %GTK_BIN%\libgtk-win32-2.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libgdk_pixbuf-2.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libgio-2.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libglib-2.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libgmodule-2.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libgobject-2.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libgthread-2.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libpng14-14.dll %XCHAT_DEST%
xcopy %GTK_BIN%\zlib1.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libgdk-win32-2.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libcairo-2.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libfontconfig-1.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libexpat-1.dll %XCHAT_DEST%
xcopy %GTK_BIN%\freetype6.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libpango-1.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libpangocairo-1.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libpangoft2-1.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libpangowin32-1.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\libatk-1.0-0.dll %XCHAT_DEST%
xcopy %GTK_BIN%\intl.dll %XCHAT_DEST%
xcopy /S /I %GTK_BIN%\..\lib\gtk-2.0 %XCHAT_DEST%\lib\gtk-2.0
rmdir /Q /S %XCHAT_DEST%\lib\gtk-2.0\include
xcopy /S /I etc %XCHAT_DEST%\etc
xcopy ..\COPYING %XCHAT_DEST%
xcopy %SSL_BIN%\..\LICENSE.OPENSSL %XCHAT_DEST%
xcopy %SSL_BIN%\..\LICENSE.ZLIB %XCHAT_DEST%
xcopy %SSL_BIN%\libeay32.dll %XCHAT_DEST%
xcopy %SSL_BIN%\ssleay32.dll %XCHAT_DEST%
::xcopy /S /I ..\plugins\dns\xcdns.dll %XCHAT_DEST%\plugins\
xcopy /S /I ..\plugins\ewc\xcewc.dll %XCHAT_DEST%\plugins\
::xcopy /S /I ..\plugins\lua\xclua.dll %XCHAT_DEST%\plugins\
xcopy /S /I ..\plugins\perl\xcperl.dll %XCHAT_DEST%\plugins\
xcopy /S /I ..\plugins\python\xcpython.dll %XCHAT_DEST%\plugins\
xcopy /S /I ..\plugins\tcl\xctcl.dll %XCHAT_DEST%\plugins\
xcopy /S /I ..\plugins\xdcc\xcxdcc.dll %XCHAT_DEST%\plugins\
::xcopy /S /I ..\plugins\xtray\xtray.dll %XCHAT_DEST%\plugins\
xcopy /S /I ..\po\locale %XCHAT_DEST%\locale
xcopy /S /I %GTK_BIN%\..\share\locale %XCHAT_DEST%\share\locale
