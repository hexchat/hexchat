@echo off
set ENCHANT_DEST=..\..\enchant-wdk-1.6.0-x86
set RELEASE_DIR=..\bin\release
rmdir /q /s %ENCHANT_DEST%
mkdir %ENCHANT_DEST%
copy ..\COPYING.LIB %ENCHANT_DEST%\LICENSE.ENCHANT
xcopy /q /s /i %RELEASE_DIR%\enchant.exe %ENCHANT_DEST%\bin\
copy %RELEASE_DIR%\enchant-lsmod.exe %ENCHANT_DEST%\bin
copy %RELEASE_DIR%\libenchant.dll %ENCHANT_DEST%\bin
xcopy /q /s /i enchant.h %ENCHANT_DEST%\include\enchant\
copy "enchant++.h" %ENCHANT_DEST%\include\enchant\
copy enchant-provider.h %ENCHANT_DEST%\include\enchant\
xcopy /q /s /i %RELEASE_DIR%\libenchant_ispell.dll %ENCHANT_DEST%\lib\enchant\
copy %RELEASE_DIR%\libenchant_ispell.exp %ENCHANT_DEST%\lib\enchant\
copy %RELEASE_DIR%\libenchant_ispell.lib %ENCHANT_DEST%\lib\enchant\
copy %RELEASE_DIR%\libenchant_myspell.dll %ENCHANT_DEST%\lib\enchant\
copy %RELEASE_DIR%\libenchant_myspell.exp %ENCHANT_DEST%\lib\enchant\
copy %RELEASE_DIR%\libenchant_myspell.lib %ENCHANT_DEST%\lib\enchant\
copy %RELEASE_DIR%\libenchant.exp %ENCHANT_DEST%\lib
copy %RELEASE_DIR%\libenchant.lib %ENCHANT_DEST%\lib
