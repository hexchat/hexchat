:: run this from a command prompt
:: TODO: change it to LocalAppdata when GLib is updated
@echo off

set DEPS_ROOT=C:\mozilla-build\hexchat
set SPELLING_DEST=..\build\spelling

rmdir /q /s %SPELLING_DEST%
mkdir %SPELLING_DEST%
xcopy /q /s /i %DEPS_ROOT%\myspell\dicts %SPELLING_DEST%\myspell
cd ..
echo [Setup] > build\spelling\hexchat-spelling.iss
echo WizardImageFile=%cd%\installer\wizardimage.bmp >> build\spelling\hexchat-spelling.iss
echo WizardSmallImageFile=%cd%\installer\wizardsmallimage.bmp >> build\spelling\hexchat-spelling.iss
cd ..\share\icons
echo SetupIconFile=%cd%\hexchat.ico >> win32\build\spelling\hexchat-spelling.iss
cd ..\..
type win32\spelling\hexchat-spelling.skel.iss >> win32\build\spelling\hexchat-spelling.iss
set PATH=%PROGRAMFILES(X86)%\Inno Setup 5
compil32 /cc win32\build\spelling\hexchat-spelling.iss
cd win32\spelling
pause
