@echo off
set OPATH=%PATH%
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\ddk
set LIB=c:\WinDDK\7600.16385.1\lib\wxp\i386;c:\WinDDK\7600.16385.1\lib\Crt\i386
set PATH=c:\WinDDK\7600.16385.1\bin\x86\x86;c:\WinDDK\7600.16385.1\bin\x86;c:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin;c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin
set DEST=..\lua-5.1.4-2-wdk-x86
call etc\luavs.bat
@echo off
echo.Press return when ready to install!
pause
set PATH=%OPATH%
rmdir /Q /S %DEST%
xcopy /S /I src %DEST%
xcopy COPYRIGHT %DEST%
del %DEST%\Makefile
echo.Finished!
pause
