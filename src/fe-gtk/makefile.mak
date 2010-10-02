include "..\makeinc.mak"

!ifdef X64
MACHINE_FLAG = /MACHINE:X64
!else
MACHINE_FLAG = /MACHINE:X86
!endif

COMLIB = ..\common\xchatcommon.lib
PROG = xchat.exe

all: $(PROG)

.c.obj:
	$(CC) $(CFLAGS) -I..\..\plugins $(GLIB) $(GTK) $<

$(PROG): $(FEGTK_OBJECTS) $(COMLIB) xchat-icon.obj
	$(LINK) /out:$(PROG) /entry:mainCRTStartup $(LDFLAGS) $(LIBS) $(FEGTK_OBJECTS) $(COMLIB) xchat-icon.obj

xchat.rc:
	echo XC_ICON ICON "../../xchat.ico" > xchat.rc

xchat.res: xchat.rc ../../xchat.ico
	rc /r xchat.rc

xchat-icon.obj: xchat.res
	cvtres /nologo $(MACHINE_FLAG) /OUT:xchat-icon.obj xchat.res

clean:
	@del *.obj
	@del $(PROG)
	@del xchat.rc
	@del xchat.res
