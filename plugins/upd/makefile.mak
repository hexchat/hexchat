include "..\..\src\makeinc.mak"

all: upd.obj upd.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcupd.dll /def:upd.def upd.obj

upd.def:
	echo EXPORTS > upd.def
	echo xchat_plugin_init >> upd.def
	echo xchat_plugin_deinit >> upd.def

upd.obj: upd.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. upd.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
