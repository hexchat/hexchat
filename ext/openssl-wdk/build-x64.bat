@echo off
set OPENSSL_DEST=..\openssl-wdk-1.0.0a-x86
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\ddk;c:\mozilla-build\build\openssl-1.0.0a\ms;c:\mozilla-build\build\zlib-x64\include
set LIB=c:\WinDDK\7600.16385.1\lib\wnet\amd64;c:\WinDDK\7600.16385.1\lib\Crt\amd64;c:\mozilla-build\build\zlib-x64\lib
set PATH=c:\WinDDK\7600.16385.1\bin\x86\amd64;c:\WinDDK\7600.16385.1\bin\x86;c:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin;c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin;c:\mozilla-build\perl-5.12-x64\bin;c:\Windows\System32;c:\mozilla-build\nasm;c:\mozilla-build\build\zlib-x64\bin
perl Configure VC-WIN64A enable-camellia no-asm zlib-dynamic --openssldir=./
call ms\do_win64a
nmake -f ms\ntdll.mak vclean
nmake -f ms\ntdll.mak
nmake -f ms\ntdll.mak test
echo.Press return when ready to install!
pause
move include include-orig
nmake -f ms\ntdll.mak install
rmdir /q /s %OPENSSL_DEST%
mkdir %OPENSSL_DEST%
move bin %OPENSSL_DEST%
move include %OPENSSL_DEST%
move lib %OPENSSL_DEST%
move openssl.cnf %OPENSSL_DEST%
move include-orig include
copy c:\mozilla-build\build\zlib-x64\bin\zlib1.dll %OPENSSL_DEST%\bin
echo.Finished!
pause
