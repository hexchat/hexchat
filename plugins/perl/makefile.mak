include "..\..\src\makeinc.mak"

TARGET = $(PERLOUTPUT)

all: $(TARGET)

perl.def:
	echo EXPORTS > perl.def
	echo xchat_plugin_init >> perl.def
	echo xchat_plugin_deinit >> perl.def
	echo xchat_plugin_get_info >> perl.def

perl.obj: perl.c
	$(CC) $(CFLAGS) perl.c $(GLIB) -I.. -I$(PERLPATH) -DPERL_DLL=\"$(PERLLIB).dll\"

perl.c: xchat.pm.h

xchat.pm.h: Xchat.pm IRC.pm
	perl.exe generate_header

$(TARGET): perl.obj perl.def
	$(LINK) /DLL /out:$(TARGET) perl.obj $(LDFLAGS) $(PERLLIB).lib /LIBPATH:$(PERLPATH) /DELAYLOAD:$(PERLLIB).dll DELAYIMP.LIB user32.lib shell32.lib advapi32.lib /def:perl.def

clean:
	del $(TARGET)
	del *.obj
	del perl.def
	del *.lib
	del *.exp
