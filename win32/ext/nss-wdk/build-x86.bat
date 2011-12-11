@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt
set LIB=%WDK_ROOT%\lib\wxp\i386;%WDK_ROOT%\lib\Crt\i386
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin;%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\Common7\IDE;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin;c:\mozilla-build\msys\bin;c:\mozilla-build\moztools\bin
set BUILD_OPT=1
set USE_64=
set WINDDK_BUILD=1
cd mozilla\security\nss
make nss_build_all
cd ..\..\..
echo.Finished!
pause
