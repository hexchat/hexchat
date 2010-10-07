@echo off
set OPENSSL_DEST=..\openssl-wdk-1.0.0a-x86
set INCLUDE=c:\WinDDK\7600.16385.1\inc\api;c:\WinDDK\7600.16385.1\inc\crt;c:\WinDDK\7600.16385.1\inc\ddk;c:\mozilla-build\build\openssl-1.0.0a\ms;c:\mozilla-build\build\zlib-x86\include
set LIB=c:\WinDDK\7600.16385.1\lib\wxp\i386;c:\WinDDK\7600.16385.1\lib\Crt\i386;c:\mozilla-build\build\zlib-x86\lib
set PATH=c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;c:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE;c:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin;c:\mozilla-build\perl-5.12-x86\bin;c:\Windows\System32;c:\mozilla-build\nasm;c:\mozilla-build\build\zlib-x86\bin
perl Configure VC-WIN32 enable-camellia zlib-dynamic --openssldir=./
call ms\do_nasm
@echo off
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
copy c:\mozilla-build\build\zlib-x86\bin\zlib1.dll %OPENSSL_DEST%\bin
move cert.pem %OPENSSL_DEST%\bin
echo.Finished!
pause
