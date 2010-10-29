include "..\..\src\makeinc.mak"

all: lua.obj lua.def
	link $(LDFLAGS) $(LIBS) /dll /out:xclua.dll $(LUALIB).lib /def:lua.def lua.obj 

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
