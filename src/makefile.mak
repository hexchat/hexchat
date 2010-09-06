all:
	@cd common
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\fe-gtk
	@-$(MAKE) /nologo /s /f makefile.mak $@
	#@cd ..\fe-text
	#@-$(MAKE) /nologo /s /f makefile.mak $@

clean:
	@del common\*.obj
	@del common\xchatcommon.lib
	@del fe-gtk\*.obj
	@del fe-gtk\mmx_cmod.o
	@del fe-gtk\xchat.exe
	@del fe-gtk\xchat.rc
	@del fe-gtk\xchat.res
	#@del fe-text\*.obj
	#@del fe-text\xchat-text.exe
	@del pixmaps\*.h
