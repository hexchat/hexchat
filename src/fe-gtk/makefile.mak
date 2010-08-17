include "..\makeinc.mak"

!ifdef X64
MACHINE_FLAG = /MACHINE:X64
!else
MACHINE_FLAG = /MACHINE:X86
!endif

COMLIB = ..\common\xchatcommon.lib
!ifdef PORTABLE
PROG = xchat-portable.exe
!else
PROG = xchat.exe
!endif

all: $(PROG)

mmx_cmod.o: mmx_cmod.S
	gcc -DUNDERSCORE_SYMBOLS -c mmx_cmod.S

.c.obj:
	$(CC) $(CFLAGS) $(GLIB) $(GTK) $(SPELL) $<

$(PROG): $(FEGTK_OBJECTS) $(COMLIB) xchat-icon.obj
	$(LINK) /out:$(PROG) /ENTRY:mainCRTStartup $(LDFLAGS) $(LIBS) $(FEGTK_OBJECTS) $(COMLIB) xchat-icon.obj
	@dir $(PROG)

xchat.rc:
	echo XC_ICON ICON "../../xchat.ico" > xchat.rc

xchat.res: xchat.rc ../../xchat.ico
	rc /r xchat.rc

xchat-icon.obj: xchat.res
	cvtres /NOLOGO $(MACHINE_FLAG) /OUT:xchat-icon.obj xchat.res

clean:
	del *.obj
	del $(PROG)
	del xchat.rc
	del xchat.RES
