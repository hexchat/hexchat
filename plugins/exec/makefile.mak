include "..\..\src\makeinc.mak"

all: exec.obj exec.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcexec.dll /def:exec.def exec.obj

exec.def:
	echo EXPORTS > exec.def
	echo xchat_plugin_init >> exec.def
	echo xchat_plugin_deinit >> exec.def

exec.obj: exec.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. exec.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
