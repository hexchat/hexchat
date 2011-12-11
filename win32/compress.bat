@echo off
set OPATH=%PATH% 
set PATH=c:\mozilla-build\mpress;c:\mozilla-build\upx
cd ..\tmp
for %%A in (*.dll) do upx -9 -q %%A
for %%A in (*.x64) do mpress -q %%A
upx xchat.exe
cd lib\enchant
for %%A in (*.dll) do upx -9 -q %%A
for %%A in (*.x64) do mpress -q %%A
:: gtk-2.0\2.10.0\engines is already packed, skip it
cd ..\gtk-2.0\modules
for %%A in (*.dll) do upx -9 -q %%A
for %%A in (*.x64) do mpress -q %%A
cd ..\..\..\plugins
for %%A in (*.dll) do upx -9 -q %%A
for %%A in (*.x64) do mpress -q %%A
cd ..\..\build
set PATH=%OPATH%
