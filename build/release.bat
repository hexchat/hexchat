@echo off
set XCHAT_X64=c:\mozilla-build\build\xchat-wdk-x64
set XCHAT_DEST=c:\mozilla-build\build\xchat-wdk-uni
move %XCHAT_X64%\xchat.exe %XCHAT_DEST%\xchat.exe.x64
move %XCHAT_X64%\libgtk-win32-2.0-0.dll %XCHAT_DEST%\libgtk-win32-2.0-0.dll.x64
move %XCHAT_X64%\libgdk_pixbuf-2.0-0.dll %XCHAT_DEST%\libgdk_pixbuf-2.0-0.dll.x64
move %XCHAT_X64%\libgio-2.0-0.dll %XCHAT_DEST%\libgio-2.0-0.dll.x64
move %XCHAT_X64%\libglib-2.0-0.dll %XCHAT_DEST%\libglib-2.0-0.dll.x64
move %XCHAT_X64%\libgmodule-2.0-0.dll %XCHAT_DEST%\libgmodule-2.0-0.dll.x64
move %XCHAT_X64%\libgobject-2.0-0.dll %XCHAT_DEST%\libgobject-2.0-0.dll.x64
move %XCHAT_X64%\libgthread-2.0-0.dll %XCHAT_DEST%\libgthread-2.0-0.dll.x64
move %XCHAT_X64%\libpng14-14.dll %XCHAT_DEST%\libpng14-14.dll.x64
move %XCHAT_X64%\zlib1.dll %XCHAT_DEST%\zlib1.dll.x64
move %XCHAT_X64%\libgdk-win32-2.0-0.dll %XCHAT_DEST%\libgdk-win32-2.0-0.dll.x64
move %XCHAT_X64%\libcairo-2.dll %XCHAT_DEST%\libcairo-2.dll.x64
move %XCHAT_X64%\libfontconfig-1.dll %XCHAT_DEST%\libfontconfig-1.dll.x64
move %XCHAT_X64%\libexpat-1.dll %XCHAT_DEST%\libexpat-1.dll.x64
move %XCHAT_X64%\libfreetype-6.dll %XCHAT_DEST%\libfreetype-6.dll.x64
move %XCHAT_X64%\libpango-1.0-0.dll %XCHAT_DEST%\libpango-1.0-0.dll.x64
move %XCHAT_X64%\libpangocairo-1.0-0.dll %XCHAT_DEST%\libpangocairo-1.0-0.dll.x64
move %XCHAT_X64%\libpangoft2-1.0-0.dll %XCHAT_DEST%\libpangoft2-1.0-0.dll.x64
move %XCHAT_X64%\libpangowin32-1.0-0.dll %XCHAT_DEST%\libpangowin32-1.0-0.dll.x64
move %XCHAT_X64%\libatk-1.0-0.dll %XCHAT_DEST%\libatk-1.0-0.dll.x64
move %XCHAT_X64%\libintl-8.dll %XCHAT_DEST%\libintl-8.dll.x64
move %XCHAT_X64%\lib\gtk-2.0\2.10.0\engines\libpixmap.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines\libpixmap.dll.x64
move %XCHAT_X64%\lib\gtk-2.0\2.10.0\engines\libwimp.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines\libwimp.dll.x64
move %XCHAT_X64%\lib\gtk-2.0\modules\libgail.dll %XCHAT_DEST%\lib\gtk-2.0\modules\libgail.dll.x64
move %XCHAT_X64%\libeay32.dll %XCHAT_DEST%\libeay32.dll.x64
move %XCHAT_X64%\ssleay32.dll %XCHAT_DEST%\ssleay32.dll.x64
move %XCHAT_X64%\plugins\xcewc.dll %XCHAT_DEST%\plugins\xcewc.dll.x64
move %XCHAT_X64%\plugins\xclua.dll %XCHAT_DEST%\plugins\xclua.dll.x64
move %XCHAT_X64%\plugins\xcperl.dll %XCHAT_DEST%\plugins\xcperl.dll.x64
move %XCHAT_X64%\plugins\xcpython.dll %XCHAT_DEST%\plugins\xcpython.dll.x64
move %XCHAT_X64%\plugins\xctcl.dll %XCHAT_DEST%\plugins\xctcl.dll.x64
move %XCHAT_X64%\plugins\xcxdcc.dll %XCHAT_DEST%\plugins\xcxdcc.dll.x64
move %XCHAT_X64%\lua51.dll %XCHAT_DEST%\lua51.dll.x64
rmdir /Q /S %XCHAT_X64%
