include "..\..\src\makeinc.mak"

all: xdcc.obj xdcc.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcxdcc.dll /def:xdcc.def xdcc.obj

xdcc.def:
	echo EXPORTS > xdcc.def
	echo xchat_plugin_init >> xdcc.def
	echo xchat_plugin_deinit >> xdcc.def

xdcc.obj: xdcc.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. xdcc.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
