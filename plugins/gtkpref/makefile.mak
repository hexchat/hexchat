include "..\..\src\makeinc.mak"

all: gtkpref.obj gtkpref.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcgtkpref.dll /def:gtkpref.def gtkpref.obj

gtkpref.def:
	echo EXPORTS > gtkpref.def
	echo hexchat_plugin_init >> gtkpref.def
	echo hexchat_plugin_deinit >> gtkpref.def

gtkpref.obj: gtkpref.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. gtkpref.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
