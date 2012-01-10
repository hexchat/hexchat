include "..\..\src\makeinc.mak"

all: xsasl.obj xsasl.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcxsasl.dll /def:xsasl.def xsasl.obj

xsasl.def:
	echo EXPORTS > xsasl.def
	echo xchat_plugin_init >> xsasl.def
	echo xchat_plugin_deinit >> xsasl.def

xsasl.obj: xsasl.c makefile.mak
	cl $(CFLAGS) $(GLIB) /I.. xsasl.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
