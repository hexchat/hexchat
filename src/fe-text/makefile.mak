include "..\makeinc.mak"

COMLIB = ..\common\xchatcommon.lib
PROG = xchat-text.exe

!ifdef X64
PLATOBJ = msvcrt_win2003.obj
!else
PLATOBJ = msvcrt_winxp.obj
!endif

all: fe-text.obj
	link /out:$(PROG) /subsystem:console /nologo $(PLATOBJ) $(LIBS) $(COMLIB) fe-text.obj

fe-text.obj: fe-text.c makefile.mak
	cl $(CFLAGS) $(GLIB) fe-text.c

clean:
	@del *.obj
	@del $(PROG)
