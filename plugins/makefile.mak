all:
	@cd checksum
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\lua
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\python
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\tcl
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\upd
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\xdcc
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\xtray
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\winamp
	@-$(MAKE) /nologo /s /f makefile.mak $@	

clean:
	@del checksum\*.def
	@del checksum\*.dll
	@del checksum\*.exp
	@del checksum\*.lib
	@del checksum\*.obj
	@del lua\*.def
	@del lua\*.dll
	@del lua\*.exp
	@del lua\*.lib
	@del lua\*.obj
	@del python\*.def
	@del python\*.dll
	@del python\*.exp
	@del python\*.lib
	@del python\*.obj
	@del tcl\*.def
	@del tcl\*.dll
	@del tcl\*.exp
	@del tcl\*.lib
	@del tcl\*.obj
	@del upd\*.def
	@del upd\*.dll
	@del upd\*.exp
	@del upd\*.lib
	@del upd\*.obj
	@del xdcc\*.def
	@del xdcc\*.dll
	@del xdcc\*.exp
	@del xdcc\*.lib
	@del xdcc\*.obj
	@del xtray\*.def
	@del xtray\*.dll
	@del xtray\*.exp
	@del xtray\*.lib
	@del xtray\*.obj
	@del winamp\*.def
	@del winamp\*.dll
	@del winamp\*.exp
	@del winamp\*.lib
	@del winamp\*.obj	
