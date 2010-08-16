include "..\..\src\makeinc.mak"

xclua.dll: lua.obj lua.def
	link $(LDFLAGS) $(LIBS) /dll /out:xclua.dll /libpath:$(LUAPATH)\lib $(LUALIB).lib /def:lua.def lua.obj 
	dir xclua.dll

lua.def:
	echo EXPORTS > lua.def
	echo xchat_plugin_init >> lua.def
	echo xchat_plugin_deinit >> lua.def

lua.obj: lua.c makefile.mak
	cl $(CFLAGS) /Dsnprintf=g_snprintf /I.. /I$(LUAPATH)\include /I.. lua.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
