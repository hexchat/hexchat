@echo off
set WDK_ROOT=c:\WinDDK\7600.16385.1
set OPATH=%PATH%
set INCLUDE=%WDK_ROOT%\inc\api;%WDK_ROOT%\inc\crt;%WDK_ROOT%\inc\ddk
set LIB=%WDK_ROOT%\lib\wxp\i386;%WDK_ROOT%\lib\Crt\i386
set PATH=%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\VC\bin;%PROGRAMFILES(X86)%\Microsoft Visual Studio 10.0\Common7\IDE
set DEST=..\lua-wdk-5.2.0-x86

set MYCOMPILE=cl /nologo /MD /O2 /W3 /c
set MYLINK=link /nologo msvcrt_winxp.obj
set MYMT=mt /nologo

cd src
%MYCOMPILE% /DLUA_BUILD_AS_DLL l*.c
del lua.obj luac.obj
%MYLINK% /DLL /out:lua52.dll l*.obj
if exist lua52.dll.manifest^
  %MYMT% -manifest lua52.dll.manifest -outputresource:lua52.dll;2
%MYCOMPILE% /DLUA_BUILD_AS_DLL lua.c
%MYLINK% /out:lua.exe lua.obj lua52.lib
if exist lua.exe.manifest^
  %MYMT% -manifest lua.exe.manifest -outputresource:lua.exe
%MYCOMPILE% luac.c
@rem del lua.obj linit.obj lbaselib.obj ldblib.obj liolib.obj lmathlib.obj^
@rem    loslib.obj ltablib.obj lstrlib.obj loadlib.obj
%MYLINK% /out:luac.exe luac.obj lua52.lib
if exist luac.exe.manifest^
  %MYMT% -manifest luac.exe.manifest -outputresource:luac.exe
del *.obj *.manifest
cd ..

echo.Press return when ready to install!
pause
set PATH=%OPATH%
rmdir /Q /S %DEST%
mkdir %DEST%
mkdir %DEST%\bin
mkdir %DEST%\lib
mkdir %DEST%\include
copy COPYRIGHT %DEST%\LICENSE.LUA
:: binaries and libraries
copy src\lua.exe %DEST%\bin
copy src\lua52.dll %DEST%\bin
copy src\lua52.exp %DEST%\bin
copy src\luac.exe %DEST%\bin
:: library
copy src\lua52.lib %DEST%\lib
:: api
copy src\lua.h %DEST%\include
copy src\luaconf.h %DEST%\include
copy src\lualib.h %DEST%\include
copy src\lauxlib.h %DEST%\include
copy src\lua.hpp %DEST%\include
echo.Finished!
pause
