set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\api\crt\stl70
set LIB=c:\WinDDK\7600.16385.1\lib\wnet\amd64;c:\WinDDK\7600.16385.1\lib\Crt\amd64
set PATH=c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\x64
nmake -f makefile.mak clean
nmake -f makefile.mak X64=1 DLL=1 MFLAGS=-MD GLIBDIR=c:\mozilla-build\build\xchat-dev64\include\glib-2.0
pause
