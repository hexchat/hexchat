AM_CFLAGS = @hexchat_def_CFLAGS@
AM_LDFLAGS = @hexchat_def_LDFLAGS@

theme_SCRIPTS = thememan.exe thememan
themedir = $(bindir)

mdtool_verbose = $(mdtool_verbose_$(V))
mdtool_verbose_ = $(mdtool_verbose_$(AM_DEFAULT_VERBOSITY))
mdtool_verbose_0 = @echo "  MDTOOL  " $@; $(MDTOOL) build $< > /dev/null;
mdtool_verbose_1 = $(MDTOOL) --verbose build $<;

thememan.exe: htm-mono.csproj
	$(mdtool_verbose)

clean-local:
	rm -f thememan.exe thememan.exe.config thememan.exe.mdb thememan Main.resources

EXTRA_DIST = thememan.in
