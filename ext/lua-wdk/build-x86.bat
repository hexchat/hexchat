@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set OPATH=%PATH%
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\ddk
set LIB=%WDK_ROOT%\lib\wxp\i386;%WDK_ROOT%\lib\Crt\i386
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin;%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\Common7\IDE
set DEST=..\lua-wdk-5.1.4-2-x86
call etc\luavs.bat
@echo off
echo.Press return when ready to install!
pause
set PATH=%OPATH%
rmdir /Q /S %DEST%
mkdir %DEST%
mkdir %DEST%\bin
mkdir %DEST%\lib
mkdir %DEST%\include
copy COPYRIGHT %DEST%\LICENSE.LUA
:: binaries and libraries
copy src\lua.exe %DEST%\bin
copy src\lua51.dll %DEST%\bin
copy src\lua51.exp %DEST%\bin
copy src\luac.exe %DEST%\bin
:: library
copy src\lua51.lib %DEST%\lib
:: api
copy src\lua.h %DEST%\include
copy src\luaconf.h %DEST%\include
copy src\lualib.h %DEST%\include
copy src\lauxlib.h %DEST%\include
copy etc\lua.hpp %DEST%\include
echo.Finished!
pause
