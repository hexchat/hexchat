:: run this from a VS x86 command prompt
@echo off

SET PACKAGE_NAME=lua-5.1.5

:: copied from etc\luavs.bat
set MYCOMPILE=cl /nologo /MD /Ox /W3 /c /D_CRT_SECURE_NO_DEPRECATE
set MYLINK=link /nologo
set MYMT=mt /nologo

cd src
%MYCOMPILE% /DLUA_BUILD_AS_DLL l*.c
del lua.obj luac.obj
%MYLINK% /DLL /out:lua51.dll l*.obj
if exist lua51.dll.manifest^
  %MYMT% -manifest lua51.dll.manifest -outputresource:lua51.dll;2
%MYCOMPILE% /DLUA_BUILD_AS_DLL lua.c
%MYLINK% /out:lua.exe lua.obj lua51.lib
if exist lua.exe.manifest^
  %MYMT% -manifest lua.exe.manifest -outputresource:lua.exe
%MYCOMPILE% l*.c print.c
del lua.obj linit.obj lbaselib.obj ldblib.obj liolib.obj lmathlib.obj^
    loslib.obj ltablib.obj lstrlib.obj loadlib.obj
%MYLINK% /out:luac.exe *.obj
if exist luac.exe.manifest^
  %MYMT% -manifest luac.exe.manifest -outputresource:luac.exe
del *.obj *.manifest
cd ..
:: end of etc\luavs.bat

set LUA_SRC=%cd%
set LUA_DEST=%cd%-x86
echo.Press return when ready to install!
pause

rmdir /q /s %LUA_DEST%
mkdir %LUA_DEST%
mkdir %LUA_DEST%\bin
mkdir %LUA_DEST%\lib
mkdir %LUA_DEST%\include
copy COPYRIGHT %LUA_DEST%\LICENSE.LUA
:: binaries and libraries
copy src\lua.exe %LUA_DEST%\bin
copy src\lua51.dll %LUA_DEST%\bin
copy src\luac.exe %LUA_DEST%\bin
:: library
copy src\lua51.lib %LUA_DEST%\lib
copy src\lua51.exp %LUA_DEST%\lib
:: api
copy src\lua.h %LUA_DEST%\include
copy src\luaconf.h %LUA_DEST%\include
copy src\lualib.h %LUA_DEST%\include
copy src\lauxlib.h %LUA_DEST%\include
copy etc\lua.hpp %LUA_DEST%\include

cd %LUA_DEST%
set PATH=%PATH%;%ProgramFiles%\7-zip
del ..\%PACKAGE_NAME%-x86.7z
7z a ..\%PACKAGE_NAME%-x86.7z *
cd %LUA_SRC%
rmdir /q /s %LUA_DEST%

echo.Finished!
pause
