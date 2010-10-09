@echo off
set OPENSSL_DEST=..\openssl-wdk-1.0.0a-x64
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\ddk;c:\mozilla-build\build\openssl-1.0.0a\ms;c:\mozilla-build\build\zlib-x64\include
set LIB=c:\WinDDK\7600.16385.1\lib\wnet\amd64;c:\WinDDK\7600.16385.1\lib\Crt\amd64;c:\mozilla-build\build\zlib-x64\lib
set PATH=c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\x64;c:\mozilla-build\perl-5.12-x64\bin;c:\Windows\System32;c:\mozilla-build\nasm;c:\mozilla-build\build\zlib-x64\bin
perl Configure VC-WIN64A enable-camellia no-asm zlib-dynamic --openssldir=./
call ms\do_win64a
nmake -f ms\ntdll.mak vclean
nmake -f ms\ntdll.mak
nmake -f ms\ntdll.mak test
perl mk-ca-bundle.pl -n
echo.Press return when ready to install!
pause
move include include-orig
nmake -f ms\ntdll.mak install
rmdir /q /s %OPENSSL_DEST%
mkdir %OPENSSL_DEST%
move bin %OPENSSL_DEST%
move include %OPENSSL_DEST%
move lib %OPENSSL_DEST%
mkdir %OPENSSL_DEST%\share
move openssl.cnf %OPENSSL_DEST%\share\openssl.cnf.example
move include-orig include
copy c:\mozilla-build\build\zlib-x64\bin\zlib1.dll %OPENSSL_DEST%\bin
move cert.pem %OPENSSL_DEST%\bin
echo.Finished!
pause
