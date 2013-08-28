MDTOOL_OPTS = --verbose

theme_SCRIPTS = thememan.exe thememan
themedir = $(bindir)

thememan.exe: htm-mono.csproj
	$(MDTOOL) $(MDTOOL_OPTS) build $<

clean-local:
	rm -f thememan.exe thememan.exe.config thememan.exe.mdb thememan Main.resources

EXTRA_DIST = thememan.in
