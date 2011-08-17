@echo off
set DEPS_ROOT=..\dep-x64
set XCHAT_DEST=..\dist-x64
rmdir /q /s %XCHAT_DEST%
mkdir %XCHAT_DEST%
echo 2> portable-mode
move portable-mode %XCHAT_DEST%
copy ..\src\fe-gtk\xchat.exe %XCHAT_DEST%
copy ..\src\fe-text\xchat-text.exe %XCHAT_DEST%
copy ..\src\gtk2-prefs\gtk2-prefs.exe %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libatk-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libcairo-2.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libexpat-1.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libfontconfig-1.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libfreetype-6.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgdk_pixbuf-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgdk-win32-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgio-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libglib-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgmodule-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgobject-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgthread-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgtk-win32-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libintl-8.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libjasper-1.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libjpeg-8.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpango-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpangocairo-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpangoft2-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpangowin32-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpixman-1-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpng15-15.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libtiff-3.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libxml2-2.dll %XCHAT_DEST%
xcopy /q /s /i %DEPS_ROOT%\lib\gtk-2.0\2.10.0\engines %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines
xcopy /q /s /i %DEPS_ROOT%\lib\gtk-2.0\modules\libgail.dll %XCHAT_DEST%\lib\gtk-2.0\modules\
xcopy /q /s /i etc %XCHAT_DEST%\etc
xcopy /q /s /i share %XCHAT_DEST%\share
xcopy /q /s /i %DEPS_ROOT%\share\themes %XCHAT_DEST%\share\themes
copy ..\COPYING %XCHAT_DEST%
copy %DEPS_ROOT%\LICENSE.OPENSSL %XCHAT_DEST%
copy %DEPS_ROOT%\LICENSE.ZLIB %XCHAT_DEST%
copy %DEPS_ROOT%\share\gettext\intl\COPYING.LIB-2.0 %XCHAT_DEST%\LICENSE.GTK
copy %DEPS_ROOT%\share\gettext\intl\COPYING.LIB-2.1 %XCHAT_DEST%\LICENSE.CAIRO
copy %DEPS_ROOT%\LICENSE.LUA %XCHAT_DEST%
copy %DEPS_ROOT%\LICENSE.ENCHANT %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libeay32.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\ssleay32.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\zlib1.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\cert.pem %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libenchant.dll %XCHAT_DEST%
xcopy /q /s /i %DEPS_ROOT%\lib\enchant\libenchant_myspell.dll %XCHAT_DEST%\lib\enchant\
xcopy /q /s /i ..\plugins\checksum\xcchecksum.dll %XCHAT_DEST%\plugins\
copy ..\plugins\lua\xclua.dll %XCHAT_DEST%\plugins
copy ..\plugins\gtkpref\xcgtkpref.dll %XCHAT_DEST%\plugins
copy ..\plugins\mpcinfo\xcmpcinfo.dll %XCHAT_DEST%\plugins
copy ..\plugins\perl\xcperl-512.dll %XCHAT_DEST%\plugins
copy ..\plugins\perl\xcperl-514.dll %XCHAT_DEST%\plugins
copy ..\plugins\python\xcpython.dll %XCHAT_DEST%\plugins
copy ..\plugins\tcl\xctcl.dll %XCHAT_DEST%\plugins
copy ..\plugins\upd\xcupd.dll %XCHAT_DEST%\plugins
copy ..\plugins\xtray\xtray.dll %XCHAT_DEST%\plugins
copy ..\plugins\winamp\xcwinamp.dll %XCHAT_DEST%\plugins
copy ..\plugins\wmpa\xcwmpa.dll %XCHAT_DEST%\plugins
copy %DEPS_ROOT%\bin\lua51.dll %XCHAT_DEST%
xcopy /q /s /i ..\po\locale %XCHAT_DEST%\locale
xcopy /q /s /i %DEPS_ROOT%\share\locale %XCHAT_DEST%\share\locale
