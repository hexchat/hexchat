include "..\..\src\makeinc.mak"

all: checksum.obj checksum.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcchecksum.dll /libpath:$(LUAPATH)\lib $(LUALIB).lib /def:checksum.def checksum.obj 

checksum.def:
	echo EXPORTS > checksum.def
	echo xchat_plugin_init >> checksum.def
	echo xchat_plugin_deinit >> checksum.def

checksum.obj: checksum.c makefile.mak
	cl $(CFLAGS) /Dsnprintf=g_snprintf /I$(LUAPATH)\include checksum.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
