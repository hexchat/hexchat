include "..\..\src\makeinc.mak"

all: checksum.obj checksum.def
	link $(LDFLAGS) $(LIBS) /dll /out:xcchecksum.dll /def:checksum.def checksum.obj 

checksum.def:
	echo EXPORTS > checksum.def
	echo xchat_plugin_init >> checksum.def
	echo xchat_plugin_deinit >> checksum.def

checksum.obj: checksum.c makefile.mak
	cl $(CFLAGS) /I.. checksum.c

clean:
	del *.obj
	del *.dll
	del *.exp
	del *.lib
