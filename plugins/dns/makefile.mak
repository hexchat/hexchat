include "..\..\src\makeinc.mak"

xcdns.dll: plugin-dns.obj dns.def
	link $(LDFLAGS) /dll /out:xcdns.dll /def:dns.def plugin-dns.obj ws2_32.lib
	dir xcdns.dll

dns.def:
	echo EXPORTS > dns.def
	echo xchat_plugin_init >> dns.def
	echo xchat_plugin_deinit >> dns.def

plugin-dns.obj: plugin-dns.c makefile.mak thread.c
	cl $(CFLAGS) /I.. -Dsnprintf=_snprintf plugin-dns.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
