include "..\..\src\makeinc.mak"

TARGET = xcfishlim.dll

CFLAGS = $(CFLAGS)

FISHLIM_OBJECTS = \
fish.obj \
irc.obj \
keystore.obj \
misc.obj \
plugin_xchat.obj

all: $(FISHLIM_OBJECTS) fishlim.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcfishlim.dll /def:fishlim.def $(FISHLIM_OBJECTS)

fishlim.def:
	echo EXPORTS > fishlim.def
	echo xchat_plugin_init >> fishlim.def
	echo xchat_plugin_deinit >> fishlim.def
	echo xchat_plugin_get_info >> fishlim.def

.c.obj:
	$(CC) $(CFLAGS) $(GLIB) /I.. /c $<

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
