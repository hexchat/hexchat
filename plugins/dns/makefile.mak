include "..\..\src\makeinc.mak"

DNS_OBJECTS = \
dns.obj \
thread.obj

all: $(DNS_OBJECTS) dns.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcdns.dll /def:dns.def $(DNS_OBJECTS)

dns.def:
	echo EXPORTS > dns.def
	echo xchat_plugin_init >> dns.def
	echo xchat_plugin_deinit >> dns.def

.c.obj:
	$(CC) $(CFLAGS) $(GLIB) /I.. /c $<

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
