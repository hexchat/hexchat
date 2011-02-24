## Process with automake to produce Makefile.in

# To create a standalone tarball of your plugin run the plugin_dist
# target.  Note: you must configure the main source tree with
# --enable-maintainer-mode

# These two must be defined.
PLUGIN=mailcheck
PLUGIN_VERSION=0.1

# This file must be in the form PLUGIN-config.h.in, it can be empty but
# must exist
EXTRA_DIST = mailcheck-config.h.in

# Remember to include this line in your Makefile.am
include @top_srcdir@/plugins/Make.plugin

libdir = $(xchatdir)/plugins

lib_LTLIBRARIES = mailcheck.la
mailcheck_la_SOURCES = mailcheck.c
mailcheck_la_LDFLAGS = -avoid-version -module 
INCLUDES = $(COMMON_CFLAGS) $(PLUGIN_INCLUDES)

DISTCLEANFILES = pg_dir/*
