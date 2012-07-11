@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set PERL_PATH=c:\mozilla-build\perl-5.12-x86\perl\bin
set NASM_PATH=c:\mozilla-build\nasm
set OPENSSL_DEST=..\openssl-wdk-1.0.1c-x86
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\ddk;ms;zlib-x86\include
set LIB=%WDK_ROOT%\lib\wxp\i386;%WDK_ROOT%\lib\Crt\i386;zlib-x86\lib
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin;%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\Common7\IDE;%PROGRAMFILES(X86)%\Microsoft SDKs\Windows\v7.0A\Bin;%PERL_PATH%;%NASM_PATH%;%SystemRoot%\System32;..\zlib-x86\bin
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
copy zlib-x86\bin\zlib1.dll %OPENSSL_DEST%\bin
move cert.pem %OPENSSL_DEST%\bin
echo.Finished!
pause
