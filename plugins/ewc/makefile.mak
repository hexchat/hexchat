include "..\..\src\makeinc.mak"

xcewc.dll: ewc.obj ewc.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcewc.dll /def:ewc.def ewc.obj
	dir xcewc.dll

ewc.def:
	echo EXPORTS > ewc.def
	echo xchat_plugin_init >> ewc.def
	echo xchat_plugin_deinit >> ewc.def

ewc.obj: ewc.c makefile.mak
	cl $(CFLAGS) /I.. ewc.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
