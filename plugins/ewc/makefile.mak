include "..\..\src\makeinc.mak"

all: ewc.obj ewc.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcewc.dll /def:ewc.def ewc.obj

ewc.def:
	echo EXPORTS > ewc.def
	echo xchat_plugin_init >> ewc.def
	echo xchat_plugin_deinit >> ewc.def

ewc.obj: ewc.c makefile.mak
	cl $(CFLAGS) ewc.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
