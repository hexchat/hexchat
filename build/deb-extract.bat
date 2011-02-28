@echo off
for %%A in (*.deb) do 7z x %%A && 7z x data.tar && del data.tar
