## Process this file with automake to produce Makefile.in

EXTRA_DIST = makefile.msc makeinc.msc fe-text/fe-text.c \
	fe-text/README fe-text/fe-text.h version-script

if DO_TEXT
text_fe = fe-text
endif

if DO_GTK
gtk_fe = fe-gtk
endif

SUBDIRS = pixmaps common $(gtk_fe) $(text_fe)
