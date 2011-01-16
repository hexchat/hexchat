include "..\..\src\makeinc.mak"

DIRENTLIB = ..\..\src\common\dirent.lib
TARGET = $(PERL512OUTPUT)

all: $(TARGET)

perl.def:
	echo EXPORTS > perl.def
	echo xchat_plugin_init >> perl.def
	echo xchat_plugin_deinit >> perl.def
	echo xchat_plugin_get_info >> perl.def

perl.obj: perl.c
	$(CC) $(CFLAGS) perl.c $(GLIB) /I$(PERL512PATH) /I.. /DPERL_DLL=\"$(PERL512LIB).dll\"

perl.c: xchat.pm.h

xchat.pm.h: lib/Xchat.pm lib/IRC.pm
	perl.exe generate_header

$(TARGET): perl.obj perl.def
	$(LINK) /DLL /out:$(TARGET) perl.obj $(LDFLAGS) $(PERL512LIB).lib /libpath:$(PERL512PATH) /DELAYLOAD:$(PERL512LIB).dll $(DIRENTLIB) DELAYIMP.LIB user32.lib shell32.lib advapi32.lib /def:perl.def

clean:
	@del $(TARGET)
	@del *.obj
	@del perl.def
	@del *.lib
	@del *.exp
