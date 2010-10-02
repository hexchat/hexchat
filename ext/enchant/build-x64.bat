@echo off
set PATH=c:\mozilla-build\mingw64\bin;c:\mozilla-build\msys\bin;c:\mozilla-build\build\xchat-dev64\bin
sh configure --exec-prefix=/usr/enchant-x64 --enable-myspell --disable-ispell --disable-aspell --disable-voikko --disable-uspell --disable-hspell --disable-zemberek
