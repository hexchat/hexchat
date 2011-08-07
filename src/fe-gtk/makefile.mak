include "..\makeinc.mak"

FEGTK_OBJECTS = \
about.obj \
ascii.obj \
banlist.obj \
chanlist.obj \
chanview.obj \
custom-list.obj \
dccgui.obj \
editlist.obj \
fe-gtk.obj \
fkeys.obj \
gtkutil.obj \
ignoregui.obj \
joind.obj \
maingui.obj \
menu.obj \
notifygui.obj \
palette.obj \
pixmaps.obj \
plugingui.obj \
plugin-tray.obj \
rawlog.obj \
search.obj \
servlistgui.obj \
setup.obj \
sexy-iso-codes.obj \
sexy-marshal.obj \
sexy-spell-entry.obj \
textgui.obj \
urlgrab.obj \
userlistgui.obj \
xtext.obj

!ifdef X64
MACHINE_FLAG = /MACHINE:X64
!else
MACHINE_FLAG = /MACHINE:X86
!endif

COMLIB = ..\common\xchatcommon.lib
PROG = xchat.exe

all: $(PROG)

.c.obj::
	$(CC) $(CFLAGS) -I..\..\plugins $(GLIB) $(GTK) $<

$(PROG): $(FEGTK_OBJECTS) $(COMLIB) xchat-icon.obj
	$(LINK) /out:$(PROG) /entry:mainCRTStartup $(LDFLAGS) $(LIBS) $(FEGTK_OBJECTS) $(COMLIB) xchat-icon.obj

xchat.res: xchat.rc ../../xchat.ico
	rc /nologo /r xchat.rc

xchat-icon.obj: xchat.res
	cvtres /nologo $(MACHINE_FLAG) /OUT:xchat-icon.obj xchat.res

clean:
	@del *.obj
	@del $(PROG)
	@del xchat.res
