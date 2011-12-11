include "..\..\src\makeinc.mak"

DIRENTLIB = ..\..\src\common\dirent-win32.lib
TARGET = $(PERL514OUTPUT)

all: $(TARGET)

perl.def:
	echo EXPORTS > perl.def
	echo xchat_plugin_init >> perl.def
	echo xchat_plugin_deinit >> perl.def
	echo xchat_plugin_get_info >> perl.def

# MSVC only supports __inline, while GCC only supports __inline__. This is defined incorretly
# in lib\CORE\config.h in Strawberry Perl, see #define PERL_STATIC_INLINE static __inline__
perl.obj: perl.c
	$(CC) $(CFLAGS) perl.c $(GLIB) /I$(PERL514PATH)\perl\lib\CORE /I.. /DPERL_DLL=\"$(PERL514LIB).dll\" /D__inline__=__inline

perl514.def:
	gendef $(PERL514PATH)\perl\bin\perl514.dll

$(PERL514LIB).lib: perl514.def
!ifdef X64
	lib /nologo /machine:x64 /def:perl514.def
!else
	lib /nologo /machine:x86 /def:perl514.def
!endif

perl.c: xchat.pm.h

xchat.pm.h: lib/Xchat.pm lib/IRC.pm
	perl.exe generate_header

$(TARGET): perl.obj perl.def $(PERL514LIB).lib
	$(LINK) /DLL /out:$(TARGET) perl.obj $(LDFLAGS) $(PERL514LIB).lib /delayload:$(PERL514LIB).dll $(DIRENTLIB) delayimp.lib user32.lib shell32.lib advapi32.lib /def:perl.def

clean:
	@del $(TARGET)
	@del *.obj
	@del *.def
	@del *.lib
	@del *.exp
