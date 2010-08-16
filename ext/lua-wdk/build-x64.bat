@echo off
set OPATH=%PATH%
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\ddk
set LIB=c:\WinDDK\7600.16385.1\lib\wnet\amd64;c:\WinDDK\7600.16385.1\lib\Crt\amd64
set PATH=c:\WinDDK\7600.16385.1\bin\x86\amd64;c:\WinDDK\7600.16385.1\bin\x86;c:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin;c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin
set DEST=..\lua-wdk-5.1.4-2-x64
call etc\luavs-x64.bat
@echo off
echo.Press return when ready to install!
pause
set PATH=%OPATH%
rmdir /Q /S %DEST%
mkdir %DEST%
mkdir %DEST%\bin
mkdir %DEST%\lib
mkdir %DEST%\include
xcopy COPYRIGHT %DEST%
:: binaries and libraries
xcopy src\lua.exe %DEST%\bin
xcopy src\lua51.dll %DEST%\bin
xcopy src\lua51.exp %DEST%\bin
xcopy src\luac.exe %DEST%\bin
:: library
xcopy src\lua51.lib %DEST%\lib
:: api
xcopy src\lua.h %DEST%\include
xcopy src\luaconf.h %DEST%\include
xcopy src\lualib.h %DEST%\include
xcopy src\lauxlib.h %DEST%\include
xcopy etc\lua.hpp %DEST%\include
echo.Finished!
pause
