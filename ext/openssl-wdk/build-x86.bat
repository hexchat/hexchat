@echo off
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\ddk;c:\mozilla-build\build\openssl-1.0.0a\ms;c:\mozilla-build\build\zlib-x86\include
set LIB=c:\WinDDK\7600.16385.1\lib\wxp\i386;c:\WinDDK\7600.16385.1\lib\Crt\i386;c:\mozilla-build\build\zlib-x86\lib
set PATH=c:\WinDDK\7600.16385.1\bin\x86\x86;c:\WinDDK\7600.16385.1\bin\x86;c:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin;c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin;c:\mozilla-build\perl-5.12-x86\bin;c:\Windows\System32;c:\mozilla-build\nasm;c:\mozilla-build\build\zlib-x86\bin
perl Configure VC-WIN32 enable-camellia zlib-dynamic --openssldir=c:/mozilla-build/build/openssl-wdk-1.0.0a-x86
call ms\do_nasm
@echo off
nmake -f ms\ntdll.mak vclean
nmake -f ms\ntdll.mak
nmake -f ms\ntdll.mak test
echo.Press return when ready to install!
pause
nmake -f ms\ntdll.mak install
perl util\copy.pl c:/mozilla-build/build/zlib-x86/bin/zlib1.dll c:/mozilla-build/build/openssl-wdk-1.0.0a-x86/bin
echo.Finished!
