include "..\..\src\makeinc.mak"

all: winsys.obj winsys.def
	link $(LDFLAGS) $(LIBS) wbemuuid.lib /dll /out:xcwinsys.dll /def:winsys.def winsys.obj

winsys.def:
	echo EXPORTS > winsys.def
	echo xchat_plugin_init >> winsys.def
	echo xchat_plugin_deinit >> winsys.def

winsys.obj: winsys.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. winsys.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
