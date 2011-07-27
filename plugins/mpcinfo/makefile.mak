include "..\..\src\makeinc.mak"

all: mpcinfo.obj mpcinfo.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcmpcinfo.dll /def:mpcinfo.def mpcinfo.obj

mpcinfo.def:
	echo EXPORTS > mpcinfo.def
	echo xchat_plugin_init >> mpcinfo.def
	echo xchat_plugin_deinit >> mpcinfo.def

mpcinfo.obj: mpcinfo.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. mpcinfo.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
