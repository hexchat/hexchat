include "..\..\src\makeinc.mak"

all: nonbmp.obj nonbmp.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcnonbmp.dll /def:nonbmp.def nonbmp.obj

nonbmp.def:
	echo EXPORTS > nonbmp.def
	echo xchat_plugin_init >> nonbmp.def
	echo xchat_plugin_deinit >> nonbmp.def

nonbmp.obj: nonbmp.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. nonbmp.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
