## Process this file with automake to produce Makefile.in

EXTRA_DIST = version-script

if DO_TEXT
text_fe = fe-text
endif

if DO_GTK
gtk_fe = fe-gtk
endif

SUBDIRS = common $(gtk_fe) $(text_fe)

if WITH_TM
SUBDIRS += htm
endif
