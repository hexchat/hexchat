include "..\..\src\makeinc.mak"

all: winamp.obj winamp.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcwinamp.dll /def:winamp.def winamp.obj

winamp.def:
	echo EXPORTS > winamp.def
	echo xchat_plugin_init >> winamp.def
	echo xchat_plugin_deinit >> winamp.def

winamp.obj: winamp.c makefile.mak
	cl $(CFLAGS) /I.. winamp.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
