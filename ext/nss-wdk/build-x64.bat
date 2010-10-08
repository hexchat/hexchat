@echo off
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\ddk
set LIB=c:\WinDDK\7600.16385.1\lib\wnet\amd64;c:\WinDDK\7600.16385.1\lib\Crt\amd64
set PATH=c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\x64;c:\mozilla-build\msys\bin;c:\mozilla-build\moztools-x64\bin
set BUILD_OPT=1
set USE_64=1
set WINDDK_BUILD=1
cd mozilla\security\nss
make nss_build_all
cd ..\..\..
echo.Finished!
pause
