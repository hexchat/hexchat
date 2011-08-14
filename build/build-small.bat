@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\api\crt\stl70;%WDK_ROOT%\inc\mfc42;%WDK_ROOT%\inc\ddk;%WDK_ROOT%\inc\api\dao360
set LIB=%WDK_ROOT%\lib\wxp\i386;%WDK_ROOT%\lib\Crt\i386;%WDK_ROOT%\lib\Mfc\i386;%WDK_ROOT%\lib\ATL\i386
set OPATH=%PATH%
set DEV_32=%cd%\..\dep-x86
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin;%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\Common7\IDE;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin;%DEV_32%\bin
del version.exe
cl /nologo version.c
cd ..
build\version -r > resource.h
cd src
echo DEV = %DEV_32% > makeinc.mak
type makeinc.skel.mak >> makeinc.mak
nmake /nologo /f makefile.mak clean
cd pixmaps
nmake /nologo /f makefile.mak
cd ..
nmake /nologo /f makefile.mak
cd ..\plugins
nmake /nologo /f makefile.mak clean
nmake /nologo /f makefile.mak
cd ..\build
set PATH=%OPATH%
set DEPS_ROOT=..\dep-x86
set XCHAT_DEST=..\tmp
rmdir /q /s %XCHAT_DEST%
mkdir %XCHAT_DEST%
echo 2> portable-mode
move portable-mode %XCHAT_DEST%
copy ..\src\fe-gtk\xchat.exe %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgtk-win32-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgdk_pixbuf-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgio-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libglib-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgmodule-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgobject-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgthread-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpng15-15.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libgdk-win32-2.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libcairo-2.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libfontconfig-1.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libexpat-1.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libfreetype-6.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libjasper-1.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libjpeg-8.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpango-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpangocairo-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpangoft2-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpangowin32-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libpixman-1-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libatk-1.0-0.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libintl-8.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libtiff-3.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libxml2-2.dll %XCHAT_DEST%
xcopy /q /s /i %DEPS_ROOT%\lib\gtk-2.0\2.10.0\engines\libpixmap.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines\
copy %DEPS_ROOT%\lib\gtk-2.0\2.10.0\engines\libwimp.dll %XCHAT_DEST%\lib\gtk-2.0\2.10.0\engines
xcopy /q /s /i %DEPS_ROOT%\lib\gtk-2.0\modules\libgail.dll %XCHAT_DEST%\lib\gtk-2.0\modules\
xcopy /q /s /i etc %XCHAT_DEST%\etc
xcopy /q /s /i share %XCHAT_DEST%\share
copy %DEPS_ROOT%\bin\libeay32.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\ssleay32.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\zlib1.dll %XCHAT_DEST%
copy %DEPS_ROOT%\bin\cert.pem %XCHAT_DEST%
copy %DEPS_ROOT%\bin\libenchant.dll %XCHAT_DEST%
xcopy /q /s /i %DEPS_ROOT%\lib\enchant\libenchant_myspell.dll %XCHAT_DEST%\lib\enchant\
xcopy /q /s /i ..\plugins\checksum\xcchecksum.dll %XCHAT_DEST%\plugins\
copy ..\plugins\lua\xclua.dll %XCHAT_DEST%\plugins
copy ..\plugins\mpcinfo\xcmpcinfo.dll %XCHAT_DEST%\plugins
copy ..\plugins\python\xcpython.dll %XCHAT_DEST%\plugins
copy ..\plugins\tcl\xctcl.dll %XCHAT_DEST%\plugins
copy ..\plugins\upd\xcupd.dll %XCHAT_DEST%\plugins
copy ..\plugins\xtray\xtray.dll %XCHAT_DEST%\plugins
copy ..\plugins\winamp\xcwinamp.dll %XCHAT_DEST%\plugins
copy ..\plugins\wmpa\xcwmpa.dll %XCHAT_DEST%\plugins
copy %DEPS_ROOT%\bin\lua51.dll %XCHAT_DEST%
pause
