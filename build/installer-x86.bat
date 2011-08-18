@echo off
set PATH=C:\Program Files (x86)\Windows Installer XML v3.5\bin
candle installer-x86.wxs -ext WixUIExtension -ext WixUtilExtension
light installer-x86.wixobj -ext WixUIExtension -ext WixUtilExtension
