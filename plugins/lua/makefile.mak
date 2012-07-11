include "..\..\src\makeinc.mak"

DIRENTLIB = ..\..\src\common\dirent-win32.lib

all: lua.obj lua.def
	link $(LDFLAGS) $(LIBS) /dll /out:xclua.dll $(LUALIB).lib $(DIRENTLIB) /def:lua.def lua.obj 

lua.def:
	echo EXPORTS > lua.def
	echo xchat_plugin_init >> lua.def
	echo xchat_plugin_deinit >> lua.def

lua.obj: lua.c makefile.mak
	cl $(CFLAGS) /I.. /Dsnprintf=g_snprintf lua.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
