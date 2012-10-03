:: run this from a VS x86 command prompt
@echo off

SET PACKAGE_NAME=openssl-1.0.1c

set OPENSSL_SRC=%cd%
set OPENSSL_DEST=%cd%-x86
set PERL_PATH=c:\mozilla-build\perl-5.16\Win32\perl\bin
set NASM_PATH=c:\mozilla-build\nasm
set INCLUDE=%INCLUDE%;%OPENSSL_SRC%\..\build\Win32\include
set LIB=%LIB%;%OPENSSL_SRC%\..\build\Win32\lib
set PATH=%PATH%;%PERL_PATH%;%NASM_PATH%;%OPENSSL_SRC%\..\build\Win32\bin
perl Configure VC-WIN32 enable-camellia zlib-dynamic --openssldir=./
call ms\do_nasm
@echo off
nmake -f ms\ntdll.mak vclean
nmake -f ms\ntdll.mak
nmake -f ms\ntdll.mak test
perl mk-ca-bundle.pl -n
echo.Press return when ready to install!
pause

:: hack to have . as openssldir which is required for having OpenSSL load cert.pem from .
move include include-orig
nmake -f ms\ntdll.mak install
rmdir /q /s %OPENSSL_DEST%
mkdir %OPENSSL_DEST%
move bin %OPENSSL_DEST%
move include %OPENSSL_DEST%
move lib %OPENSSL_DEST%
mkdir %OPENSSL_DEST%\share
mkdir %OPENSSL_DEST%\share\doc
mkdir %OPENSSL_DEST%\share\doc\openssl
move openssl.cnf %OPENSSL_DEST%\share\openssl.cnf.example
move include-orig include
move cert.pem %OPENSSL_DEST%\bin
copy LICENSE %OPENSSL_DEST%\share\doc\openssl\COPYING

cd %OPENSSL_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %OPENSSL_SRC%
rmdir /q /s %OPENSSL_DEST%

echo.Finished!
pause
