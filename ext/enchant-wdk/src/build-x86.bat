set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\api\crt\stl70
set LIB=c:\WinDDK\7600.16385.1\lib\wxp\i386;c:\WinDDK\7600.16385.1\lib\Crt\i386
set PATH=c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin;c:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE
nmake -f makefile.mak clean
nmake -f makefile.mak DLL=1 MFLAGS=-MD GLIBDIR=c:\mozilla-build\build\xchat-dev32\include\glib-2.0
pause
