@echo off
echo [Setup] > xchat-wdk-spell.iss
echo WizardImageFile=%cd%\bitmaps\wizardimage.bmp >> xchat-wdk-spell.iss
echo WizardSmallImageFile=%cd%\bitmaps\wizardsmallimage.bmp >> xchat-wdk-spell.iss
cd ..
echo SetupIconFile=%cd%\xchat.ico >> build\xchat-wdk-spell.iss
type build\xchat-wdk-spell.skel.iss >> build\xchat-wdk-spell.iss
set PATH=%PROGRAMFILES(X86)%\Inno Setup 5
compil32 /cc build\xchat-wdk-spell.iss
pause
