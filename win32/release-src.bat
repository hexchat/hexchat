@echo off
set PATH=%PATH%;%PROGRAMW6432%\7-Zip;%PROGRAMFILES(X86)%\Git\bin
git clone https://github.com/hexchat/hexchat.git hexchat
rmdir /q /s hexchat\.git
7z a hexchat-head.7z hexchat
rmdir /q /s hexchat
pause
