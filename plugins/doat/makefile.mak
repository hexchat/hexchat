include "..\..\src\makeinc.mak"

all: doat.obj doat.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcdoat.dll /def:doat.def doat.obj

doat.def:
	echo EXPORTS > doat.def
	echo xchat_plugin_init >> doat.def
	echo xchat_plugin_deinit >> doat.def

doat.obj: doat.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. doat.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
