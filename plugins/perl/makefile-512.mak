include "..\..\src\makeinc.mak"

DIRENTLIB = ..\..\src\common\dirent-win32.lib
TARGET = $(PERL512OUTPUT)

all: $(TARGET)

perl.def:
	echo EXPORTS > perl.def
	echo xchat_plugin_init >> perl.def
	echo xchat_plugin_deinit >> perl.def
	echo xchat_plugin_get_info >> perl.def

perl.obj: perl.c
	$(CC) $(CFLAGS) perl.c $(GLIB) /I$(PERL512PATH)\perl\lib\CORE /I.. /DPERL_DLL=\"$(PERL512LIB).dll\"

perl512.def:
	gendef $(PERL512PATH)\perl\bin\perl512.dll

$(PERL512LIB).lib: perl512.def
!ifdef X64
	lib /nologo /machine:x64 /def:perl512.def
!else
	lib /nologo /machine:x86 /def:perl512.def
!endif

perl.c: xchat.pm.h

xchat.pm.h: lib/Xchat.pm lib/IRC.pm
	perl.exe generate_header

$(TARGET): perl.obj perl.def $(PERL512LIB).lib
	$(LINK) /DLL /out:$(TARGET) perl.obj $(LDFLAGS) $(PERL512LIB).lib /delayload:$(PERL512LIB).dll $(DIRENTLIB) delayimp.lib user32.lib shell32.lib advapi32.lib /def:perl.def

clean:
	@del $(TARGET)
	@del *.obj
	@del *.def
	@del *.lib
	@del *.exp
