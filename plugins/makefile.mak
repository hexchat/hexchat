all:
	@cd ewc
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\lua
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\python
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\tcl
	@-$(MAKE) /nologo /s /f makefile.mak $@
	@cd ..\xdcc
	@-$(MAKE) /nologo /s /f makefile.mak $@

clean:
	@del ewc\*.def
	@del ewc\*.dll
	@del ewc\*.exp
	@del ewc\*.lib
	@del ewc\*.obj
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
	@del xdcc\*.def
	@del xdcc\*.dll
	@del xdcc\*.exp
	@del xdcc\*.lib
	@del xdcc\*.obj
