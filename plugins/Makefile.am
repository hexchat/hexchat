# temp. solution while mailcheck & xdcc arn't in SUBDIRS
EXTRA_DIST = plugin20.html Make.plugin plugin-conf.in \
				mailcheck/mailcheck.c \
				mailcheck/mailcheck-config.h.in \
				mailcheck/Makefile.am \
				xdcc/xdcc.c \
				xdcc/Makefile.am

noinst_HEADERS = xchat-plugin.h

if DO_PYTHON
pythondir = python
endif

if DO_PERL
perldir = perl
endif

if DO_TCL
tcldir = tcl
endif

#SUBDIRS = . $(pythondir) $(perldir) mailcheck xdcc
SUBDIRS = . $(pythondir) $(perldir) $(tcldir)
