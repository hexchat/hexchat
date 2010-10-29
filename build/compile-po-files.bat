@echo off
echo.Compiling translations . . .
cd ..\po
rmdir /q /s locale
mkdir locale
for %%A in (am az be bg ca cs de el en_GB es et eu fi fr gl hi hu it ja kn ko lt lv mk ms nb nl no pa pl pt pt_BR ru sk sl sq sr sv th uk vi wa zh_CN zh_TW) do mkdir locale\%%A\LC_MESSAGES && msgfmt -co locale\%%A\LC_MESSAGES\xchat.mo %%A.po
cd ..\build
