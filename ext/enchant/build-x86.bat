@echo off
set PATH=c:\mozilla-build\mingw32\bin;c:\mozilla-build\msys\bin;c:\mozilla-build\build\xchat-dev32\bin
sh configure --exec-prefix=/usr/enchant-x86 --enable-myspell --disable-ispell --disable-aspell --disable-voikko --disable-uspell --disable-hspell --disable-zemberek
