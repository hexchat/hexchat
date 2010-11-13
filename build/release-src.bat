@echo off
set PATH=%PATH%;%PROGRAMW6432%\7-Zip;%PROGRAMW6432%\TortoiseSVN\bin;%PROGRAMFILES(X86)%\TortoiseHg
hg clone https://xchat-wdk.googlecode.com/hg/ xchat-wdk
rmdir /q /s xchat-wdk\.hg
TortoiseProc /closeonend:0 /notempfile /command:export /url:https://xchat.svn.sourceforge.net/svnroot/xchat /path:xchat-wdk-svn
rm xchat-wdk-svn\COPYING
xcopy /E xchat-wdk-svn xchat-wdk
rmdir /q /s xchat-wdk-svn
cd xchat-wdk
patch -i xchat-wdk.patch -p1
cd ..
7z a xchat-wdk-tip.7z xchat-wdk
rmdir /q /s xchat-wdk
pause
