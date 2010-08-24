all:
	copy ..\plugins\xchat-plugin.h common
	@cd common
	@-$(MAKE) -f makefile.mak $@
	@cd ..\fe-gtk
	@-$(MAKE) -f makefile.mak $@

clean:
	del common\*.obj
	del common\xchatcommon.lib
	del fe-gtk\*.obj
	del fe-gtk\mmx_cmod.o
	del fe-gtk\xchat.exe
	del fe-gtk\xchat.rc
	del fe-gtk\xchat.RES
	del pixmaps\*.h
