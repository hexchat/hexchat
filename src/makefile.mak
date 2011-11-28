all:
	@cd pixmaps
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\common
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\fe-gtk
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\fe-text
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\gtk2-prefs
	@-$(MAKE) /nologo /s /f makefile.mak $@

clean:
	@cd pixmaps
	@-$(MAKE) /nologo /s /f makefile.mak clean $@
	@cd ..\common
	@-$(MAKE) /nologo /s /f makefile.mak clean $@
	@cd ..\fe-gtk
	@-$(MAKE) /nologo /s /f makefile.mak clean $@
	@cd ..\fe-text
	@-$(MAKE) /nologo /s /f makefile.mak clean $@
	@cd ..\gtk2-prefs
	@-$(MAKE) /nologo /s /f makefile.mak clean $@
