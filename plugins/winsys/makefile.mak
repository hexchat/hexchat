include "..\..\src\makeinc.mak"

all: winsys.obj winsys.def
	link $(LDFLAGS) $(LIBS) /NODEFAULTLIB:comsupp.lib wbemuuid.lib vccomsup.lib /dll /out:xcwinsys.dll /def:winsys.def winsys.obj

winsys.def:
	echo EXPORTS > winsys.def
	echo xchat_plugin_init >> winsys.def
	echo xchat_plugin_deinit >> winsys.def

winsys.obj: winsys.cpp makefile.mak
	cl $(CFLAGS) $(GLIB) /DUNICODE /D_UNICODE /Zc:wchar_t- /I.. winsys.cpp

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
