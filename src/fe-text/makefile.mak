include "..\makeinc.mak"

COMLIB = ..\common\xchatcommon.lib
PROG = xchat-text.exe

all: fe-text.obj
	link /out:$(PROG) /entry:mainCRTStartup $(LDFLAGS) $(LIBS) $(COMLIB) fe-text.obj

fe-text.obj: fe-text.c makefile.mak
	cl $(CFLAGS) $(GLIB) fe-text.c

clean:
	del *.obj
	del $(PROG)
