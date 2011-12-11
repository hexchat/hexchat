# -*- Makefile -*- for libenchant
#
# WDK fixes by Berke Viktor, 2010
#
# This makefile targets the Microsoft Visual C++ platform and is intended to build the enchant library along with its executables and test programs.
# It was written by Tolon (alex@tolon.co.uk). He thinks it was originally based on an MSVC makefile from glib, but can't quite remember.
# Tolon surrenders all rights regarding this file to the enchant project (http://www.abisource.com/enchant).
# Please direct any comments, bug-fixes, etc. to alex@tolon.co.uk.
#
# Example of my usage of this makefile with MSVC9:
#   NMAKE -f Makefile.msvc DLL=1 PREFIX=C:\usr-msvc9 GLIBDIR=D:\dev\lib\glib
#
# Example of my usage of this makefile with MSVC6:
#   NMAKE -f Makefile.msvc DLL=1 PREFIX=C:\usr-msvc6 GLIBDIR=D:\dev\lib\glib MANIFEST=0
#
# This makefile expects glib-2.0.lib and gmodule-2.0.lib to be available in the $(PREFIX)\lib folder.

ENCHANT_MAJOR_VERSION=1
ENCHANT_MINOR_VERSION=6
ENCHANT_MICRO_VERSION=0
BUILDNUMBER=0
ENCHANT_VERSION="$(ENCHANT_MAJOR_VERSION).$(ENCHANT_MINOR_VERSION).$(ENCHANT_MICRO_VERSION)"

#### Start of system configuration section. ####

# Flags that can be set on the nmake command line:
#   DLL=1     for compiling a .dll with a stub .lib (default is a static .lib)
#             Note that this works only with MFLAGS=-MD.
#   MFLAGS={-ML|-MT|-MD} for defining the compilation model
#     MFLAGS=-ML (the default)  Single-threaded, statically linked - libc.lib
#     MFLAGS=-MT                Multi-threaded, statically linked  - libcmt.lib
#     MFLAGS=-MD                Multi-threaded, dynamically linked - msvcrt.lib
#   DEBUG=1   for compiling with debugging information
#   PREFIX=Some\Directory       Base directory for installation
#   IIPREFIX=Some\\Directory    Same thing with doubled backslashes
#   GLIBDIR=Some\Directory      Path to glib include directory
#   MANIFEST=0                  Disables embedding of manifest
!if !defined(DLL)
DLL=0
!endif
!if !defined(DEBUG)
DEBUG=0
!endif
!if !defined(MFLAGS)
!if !$(DLL)
MFLAGS= 
!else
!if !$(DEBUG)
MFLAGS=-MD
!else
MFLAGS=-MDd
!endif
!endif
!endif
!if !defined(PREFIX)
PREFIX = c:\usr
!endif
!if !defined(IIPREFIX)
IIPREFIX = c:\\usr
!endif

!if !defined(MANIFEST)
MANIFEST=1
!endif

# Directories used by "make":
glibdir = $(GLIBDIR)
rootdir = .\..
topsrcdir = $(rootdir)\src
toptestdir = $(rootdir)\tests
bindir=$(rootdir)\bin
!if $(DEBUG)
outdir=$(rootdir)\bin\debug
!else
outdir=$(rootdir)\bin\release
!endif
objdir=$(outdir)\obj
pdbdir=$(outdir)\pdb
libdir=$(outdir)

# Directories used by "make install":
prefix = $(PREFIX)
exec_prefix = $(prefix)
#bindir = $(exec_prefix)\bin
#otherlibdir = $(exec_prefix)\lib
otherlibdir = $(GLIBDIR)\..\..\lib
includedir = $(prefix)\include
#datadir = $(prefix)\share
#localedir = $(datadir)\locale
#aliaspath =
#IIprefix = $(IIPREFIX)
#IIexec_prefix = $(IIprefix)
#IIbindir = $(IIexec_prefix)\\bin
#IIlibdir = $(IIexec_prefix)\\lib
#IIincludedir = $(IIprefix)\\include
#IIdatadir = $(IIprefix)\\share
#IIlocaledir = $(IIdatadir)\\locale
#IIaliaspath =

# Programs used by "make":

CC = cl
LINK = link
MT = mt

# Set to -W3 if you want to see maximum amount of warnings, including stupid
# ones. Set to -W1 to avoid warnings about signed/unsigned combinations.
WARN_CFLAGS = -W1

!if $(DEBUG)
OPTIMFLAGS = -Od
!else
# Some people prefer -O2 -G6 instead of -O1, but -O2 is not reliable in MSVC5.
OPTIMFLAGS = -DNDEBUG -D_NDEBUG -Ox -MP2
!endif

OUTPUTFLAGS = \
    -Fo"$(objdir)\\" 

WINFLAGS = \
    -DWINDOWS \
    -D_WINDOWS \
    -DWIN32 \
    -D_WIN32 \
    -DUNICODE \
    -D_UNICODE

DEBUGFLAGS = -Zi

EXCEPTIONFLAGS = -EHsc

CPPFLAGS = -D_STL70_ -D_STATIC_CPPLIB

CFLAGS = \
    -FI "$(topsrcdir)\config.h" \
    $(MFLAGS) \
    $(WARN_CFLAGS) \
    $(EXCEPTIONFLAGS) \
    $(OPTIMFLAGS) \
	$(CPPFLAGS) \
    $(WINFLAGS) \
    $(OUTPUTFLAGS)

INCLUDES = \
    -I$(topsrcdir) \
    -I$(rootdir) \
    -I$(includedir) \
    -I$(glibdir) \
    -I$(glibdir)\glib \
    -I$(glibdir)\gmodule \
	-I$(glibdir)\..\..\lib\glib-2.0\include

!ifdef X64
LINKOBJ = msvcrt_win2003.obj
MACHINE_FLAG = X64
CFLAGS = $(CFLAGS) /favor:AMD64 /D_WIN64
!else
MACHINE_FLAG = X86
LINKOBJ = msvcrt_winxp.obj
!endif

LINKFLAGS = \
    -INCREMENTAL:NO \
    -MANIFEST \
    -MANIFESTUAC:"level='asInvoker' uiAccess='false'" \
    -PDB:$(pdbdir)\\ \
    -MACHINE:$(MACHINE_FLAG) \
    -OPT:REF \
    -OPT:ICF \
    -DEBUG \
	$(LINKOBJ)

AR = lib
AR_FLAGS = /out:

LN = copy
RM = -del

# Programs used by "make install":
INSTALL = copy
INSTALL_PROGRAM = copy
INSTALL_DATA = copy

#### End of system configuration section. ####

SHELL = /bin/sh

CC_OBJ = $(CC) $(INCLUDES) $(CFLAGS) -c 

CC_LINK = $(LINK) $(LINKFLAGS) 
LINK_EXE = $(CC_LINK) /SUBSYSTEM:CONSOLE
LINK_DLL = $(CC_LINK) /DLL /SUBSYSTEM:WINDOWS

!if $(MANIFEST)
EMBED_MANIFEST = $(MT) /manifest $(outdir)\exe_name.manifest /outputresource:"$(outdir)\exe_name;1"
CLEAN_MANIFEST = $(RM) $(outdir)\exe_name.manifest
!else
EMBED_MANIFEST = 
CLEAN_MANIFEST =
!endif

# test-enchantxx isn't part of all as it has problems on MSVC6.
all: makedirs libenchant libenchant_ispell libenchant_myspell enchant enchant_lsmod test-enchant
#all: makedirs libenchant libenchant_ispell enchant enchant_lsmod test-enchant

makedirs: force
    -mkdir $(bindir)
    -mkdir $(outdir)
    -mkdir $(libdir)
    -mkdir $(objdir)
    -mkdir $(pdbdir)

################################################################################
#### ENCHANT ####

srcdir = $(toptestdir)

ENCHANT_EXE=$(outdir)\enchant.exe
ENCHANT_PDB=$(pdbdir)\enchant.pdb

ENCHANT_DEFINES = \
    -D_CONSOLE \
    -DVERSION=\"$(ENCHANT_VERSION)\"
    
ENCHANT_LIBS = \
    $(otherlibdir)\glib-2.0.lib \
    $(libdir)\libenchant.lib

ENCHANT_OBJECTS = \
    $(objdir)\enchant-ispell.obj

$(objdir)\enchant-ispell.obj : $(srcdir)\enchant-ispell.c
	$(CC_OBJ) $** $(ENCHANT_DEFINES)

enchant: $(ENCHANT_EXE)

$(ENCHANT_EXE): $(ENCHANT_OBJECTS)
	$(LINK_EXE) $(ENCHANT_OBJECTS) $(ENCHANT_LIBS) -OUT:$(ENCHANT_EXE)
    $(EMBED_MANIFEST:exe_name=enchant.exe)
    $(CLEAN_MANIFEST:exe_name=enchant.exe)
    
enchant_clean:
    $(RM) $(ENCHANT_OBJECTS) $(ENCHANT_EXE) $(ENCHANT_PDB)

################################################################################
#### ENCHANT-LSMOD ####

srcdir = $(toptestdir)

ENCHANT_LSMOD_EXE=$(outdir)\enchant-lsmod.exe
ENCHANT_LSMOD_PDB=$(pdbdir)\enchant-lsmod.pdb

ENCHANT_LSMOD_DEFINES = \
    -D_CONSOLE \
    -DVERSION=\"$(ENCHANT_VERSION)\"
    
ENCHANT_LSMOD_LIBS = \
    $(otherlibdir)\glib-2.0.lib \
    $(libdir)\libenchant.lib

ENCHANT_LSMOD_OBJECTS = \
    $(objdir)\enchant-lsmod.obj

$(objdir)\enchant-lsmod.obj : $(srcdir)\enchant-lsmod.c
	$(CC_OBJ) $** $(ENCHANT_LSMOD_DEFINES)

enchant_lsmod: $(ENCHANT_LSMOD_EXE) 

$(ENCHANT_LSMOD_EXE): $(ENCHANT_LSMOD_OBJECTS)
    $(LINK_EXE) $(ENCHANT_LSMOD_OBJECTS) $(ENCHANT_LSMOD_LIBS) -OUT:$(ENCHANT_LSMOD_EXE)
    $(EMBED_MANIFEST:exe_name=enchant-lsmod.exe)
    $(CLEAN_MANIFEST:exe_name=enchant-lsmod.exe)
    
enchant_lsmod_clean: 
    $(RM) $(ENCHANT_LSMOD_OBJECTS) $(ENCHANT_LSMOD_EXE) $(ENCHANT_LSMOD_PDB)

################################################################################
#### TEST-ENCHANT ####

srcdir = $(toptestdir)

TEST_ENCHANT_EXE=$(outdir)\test-enchant.exe
TEST_ENCHANT_PDB=$(pdbdir)\test-enchant.pdb

TEST_ENCHANT_DEFINES = \
    -D_CONSOLE \
    -DVERSION=\"$(ENCHANT_VERSION)\"
    
TEST_ENCHANT_LIBS = \
    $(otherlibdir)\glib-2.0.lib \
    $(libdir)\libenchant.lib

TEST_ENCHANT_OBJECTS = \
    $(objdir)\test-enchant.obj

$(objdir)\test-enchant.obj : $(srcdir)\test-enchant.c
	$(CC_OBJ) $** $(TEST_ENCHANT_DEFINES)

test-enchant: $(TEST_ENCHANT_EXE) 

$(TEST_ENCHANT_EXE): $(TEST_ENCHANT_OBJECTS)
    $(LINK_EXE) $(TEST_ENCHANT_OBJECTS) $(TEST_ENCHANT_LIBS) -OUT:$(TEST_ENCHANT_EXE)
    $(EMBED_MANIFEST:exe_name=test-enchant.exe)
    $(CLEAN_MANIFEST:exe_name=test-enchant.exe)
    
test-enchant_clean:
    $(RM) $(TEST_ENCHANT_OBJECTS) $(TEST_ENCHANT_EXE) $(TEST_ENCHANT_PDB)

################################################################################
#### TEST-ENCHANTXX ####

srcdir = $(toptestdir)

TEST_ENCHANTXX_EXE=$(outdir)\test-enchantxx.exe
TEST_ENCHANTXX_PDB=$(pdbdir)\test-enchantxx.pdb

TEST_ENCHANTXX_DEFINES = \
    -D_CONSOLE \
    -DVERSION=\"$(ENCHANT_VERSION)\"
    
TEST_ENCHANTXX_LIBS = \
    $(otherlibdir)\glib-2.0.lib \
    $(libdir)\libenchant.lib

TEST_ENCHANTXX_OBJECTS = \
    $(objdir)\test-enchantxx.obj

$(objdir)\test-enchantxx.obj : $(srcdir)\test-enchantxx.cpp
	$(CC_OBJ) $** $(TEST_ENCHANTXX_DEFINES)

test-enchantxx: $(TEST_ENCHANTXX_EXE) 

$(TEST_ENCHANTXX_EXE): $(TEST_ENCHANTXX_OBJECTS)
    $(LINK_EXE) $(TEST_ENCHANTXX_OBJECTS) $(TEST_ENCHANTXX_LIBS) -OUT:$(TEST_ENCHANTXX_EXE)
    $(EMBED_MANIFEST:exe_name=test-enchantxx.exe)
    $(CLEAN_MANIFEST:exe_name=test-enchantxx.exe)
    
test-enchantxx_clean:
    $(RM) $(TEST_ENCHANTXX_OBJECTS) $(TEST_ENCHANTXX_EXE) $(TEST_ENCHANTXX_PDB)

################################################################################
#### LIBENCHANT ####

srcdir = $(topsrcdir)

LIBENCHANT_DEFINES = \
    -D_ENCHANT_BUILD=1 \
    -DWINDLL

LIBENCHANT_DLL=$(outdir)\libenchant.dll
LIBENCHANT_LIB=$(outdir)\libenchant.lib
LIBENCHANT_PDB=$(pdbdir)\libenchant.pdb
LIBENCHANT_RES=$(srcdir)\libenchant.res
LIBENCHANT_RC=$(srcdir)\libenchant.rc

LIBENCHANT_LIBS = \
    $(otherlibdir)\glib-2.0.lib \
    $(otherlibdir)\gmodule-2.0.lib \
    advapi32.lib

LIBENCHANT_OBJECTS = \
    $(objdir)\enchant.obj \
    $(objdir)\prefix.obj \
    $(objdir)\pwl.obj



$(objdir)\pwl.obj : $(srcdir)\pwl.c
	$(CC_OBJ) $** $(LIBENCHANT_DEFINES)

$(objdir)\prefix.obj : $(srcdir)\prefix.c
	$(CC_OBJ) $** $(LIBENCHANT_DEFINES)

$(objdir)\enchant.obj : $(srcdir)\enchant.c 
	$(CC_OBJ) $** $(LIBENCHANT_DEFINES)
    
$(LIBENCHANT_RES) : $(LIBENCHANT_RC)
	rc -Fo $(LIBENCHANT_RES) $(LIBENCHANT_RC)
    
!if !$(DLL)
libenchant: LIBENCHANT_LIB
libenchant_clean: libenchant_lib_clean
!else
libenchant: LIBENCHANT_DLL
libenchant_clean: libenchant_dll_clean
!endif

LIBENCHANT_LIB : $(LIBENCHANT_OBJECTS)
	-$(RM) libenchant.lib
	$(AR) $(AR_FLAGS)$(LIBENCHANT_LIB) $(LIBENCHANT_OBJECTS)

# libenchant.dll and libenchant.lib are created together.
LIBENCHANT_DLL : $(LIBENCHANT_OBJECTS) $(LIBENCHANT_RES)
	$(LINK_DLL) $(LIBENCHANT_OBJECTS) $(LIBENCHANT_LIBS) $(LIBENCHANT_RES) -OUT:$(LIBENCHANT_DLL)
    $(EMBED_MANIFEST:exe_name=libenchant.dll)
    $(CLEAN_MANIFEST:exe_name=libenchant.dll)

libenchant_lib_clean:
    $(RM) $(LIBENCHANT_OBJECTS) $(LIBENCHANT_LIB)

libenchant_dll_clean:
    $(RM) $(LIBENCHANT_OBJECTS) $(LIBENCHANT_LIB) $(LIBENCHANT_DLL) $(LIBENCHANT_PDB) $(LIBENCHANT_RES) $(LIBENCHANT_RC)

################################################################################
#### LIBENCHANT_ISPELL ####

srcdir = $(topsrcdir)\ispell

LIBENCHANT_ISPELL_DEFINES = \
    -D "_ENCHANT_BUILD=1" \
    -D "ISPELL_PROVIDER_EXPORTS" \
    -D_USRDLL \
    -D_WINDLL

LIBENCHANT_ISPELL_DLL=$(outdir)\libenchant_ispell.dll
LIBENCHANT_ISPELL_LIB=$(libdir)\libenchant_ispell.lib
LIBENCHANT_ISPELL_PDB=$(pdbdir)\libenchant_ispell.pdb

LIBENCHANT_ISPELL_LIBS = \
    $(otherlibdir)\glib-2.0.lib \
    $(otherlibdir)\gmodule-2.0.lib \
    $(libdir)\libenchant.lib \
    advapi32.lib ntstc_msvcrt.lib

LIBENCHANT_ISPELL_OBJECTS = \
    $(objdir)\correct.obj \
    $(objdir)\good.obj \
    $(objdir)\hash.obj \
    $(objdir)\ispell_checker.obj \
    $(objdir)\lookup.obj \
    $(objdir)\makedent.obj \
    $(objdir)\tgood.obj

!if !$(DLL)
libenchant_ispell: LIBENCHANT_ISPELL_LIB
libenchant_ispell_clean: libenchant_ispell_lib_clean
!else
libenchant_ispell: LIBENCHANT_ISPELL_DLL
libenchant_ispell_clean: libenchant_ispell_dll_clean
!endif

$(objdir)\correct.obj : $(srcdir)\correct.cpp
	$(CC_OBJ) $** $(LIBENCHANT_ISPELL_DEFINES)

$(objdir)\good.obj : $(srcdir)\good.cpp
	$(CC_OBJ) $** $(LIBENCHANT_ISPELL_DEFINES)

$(objdir)\hash.obj : $(srcdir)\hash.cpp
	$(CC_OBJ) $** $(LIBENCHANT_ISPELL_DEFINES)

$(objdir)\ispell_checker.obj : $(srcdir)\ispell_checker.cpp
	$(CC_OBJ) $** $(LIBENCHANT_ISPELL_DEFINES)
   
$(objdir)\lookup.obj : $(srcdir)\lookup.cpp
	$(CC_OBJ) $** $(LIBENCHANT_ISPELL_DEFINES)
    
$(objdir)\makedent.obj : $(srcdir)\makedent.cpp
	$(CC_OBJ) $** $(LIBENCHANT_ISPELL_DEFINES)
    
$(objdir)\tgood.obj : $(srcdir)\tgood.cpp
	$(CC_OBJ) $** $(LIBENCHANT_ISPELL_DEFINES)
    
LIBENCHANT_ISPELL_LIB : $(LIBENCHANT_ISPELL_OBJECTS)
	-$(RM) $(LIBENCHANT_ISPELL_LIB)
	$(AR) $(AR_FLAGS)$(LIBENCHANT_ISPELL_LIB) $(LIBENCHANT_ISPELL_OBJECTS)

# libenchant_ispell.dll and libenchant_ispell.lib are created together.
LIBENCHANT_ISPELL_DLL : $(LIBENCHANT_ISPELL_OBJECTS)
    $(LINK_DLL) $(LIBENCHANT_ISPELL_OBJECTS) $(LIBENCHANT_ISPELL_LIBS) -OUT:$(LIBENCHANT_ISPELL_DLL)
    $(EMBED_MANIFEST:exe_name=libenchant_ispell.dll)
    $(CLEAN_MANIFEST:exe_name=libenchant_ispell.dll)
    
libenchant_ispell_lib_clean:
    $(RM) $(LIBENCHANT_ISPELL_OBJECTS) $(LIBENCHANT_ISPELL_LIB)

libenchant_ispell_dll_clean:
    $(RM) $(LIBENCHANT_ISPELL_OBJECTS) $(LIBENCHANT_ISPELL_LIB) $(LIBENCHANT_ISPELL_DLL) $(LIBENCHANT_ISPELL_PDB)

################################################################################
#### LIBENCHANT_MYSPELL ####

srcdir = $(topsrcdir)\myspell

LIBENCHANT_MYSPELL_DEFINES = \
    -D "HUNSPELL_STATIC" \
    -D "_ENCHANT_BUILD=1" \
    -D "_USRDLL" \
    -D "MYSPELL_PROVIDER_EXPORTS" \
    -D "_WINDLL"
    
LIBENCHANT_MYSPELL_DLL=$(outdir)\libenchant_myspell.dll
LIBENCHANT_MYSPELL_LIB=$(libdir)\libenchant_myspell.lib
LIBENCHANT_MYSPELL_PDB=$(pdbdir)\libenchant_myspell.pdb

LIBENCHANT_MYSPELL_LIBS = \
    $(otherlibdir)\glib-2.0.lib \
    $(otherlibdir)\gmodule-2.0.lib \
    $(libdir)\libenchant.lib \
    advapi32.lib ntstc_msvcrt.lib

LIBENCHANT_MYSPELL_OBJECTS = \
    $(objdir)\affentry.obj \
    $(objdir)\affixmgr.obj \
    $(objdir)\dictmgr.obj \
    $(objdir)\csutil.obj \
    $(objdir)\utf_info.obj \
    $(objdir)\hashmgr.obj \
    $(objdir)\suggestmgr.obj \
    $(objdir)\hunspell.obj \
    $(objdir)\filemgr.obj \
    $(objdir)\phonet.obj \
    $(objdir)\hunzip.obj \
    $(objdir)\myspell_checker.obj
        
!if !$(DLL)
libenchant_myspell: LIBENCHANT_MYSPELL_LIB
libenchant_myspell_clean: libenchant_myspell_lib_clean
!else
libenchant_myspell: LIBENCHANT_MYSPELL_DLL
libenchant_myspell_clean: libenchant_myspell_dll_clean
!endif

$(objdir)\affentry.obj : $(srcdir)\affentry.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)

$(objdir)\affixmgr.obj : $(srcdir)\affixmgr.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)

$(objdir)\dictmgr.obj : $(srcdir)\dictmgr.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)

$(objdir)\csutil.obj : $(srcdir)\csutil.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)

$(objdir)\utf_info.obj : $(srcdir)\utf_info.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)
    
$(objdir)\hashmgr.obj : $(srcdir)\hashmgr.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)
    
$(objdir)\suggestmgr.obj : $(srcdir)\suggestmgr.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)
    
$(objdir)\hunspell.obj : $(srcdir)\hunspell.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)
    
$(objdir)\filemgr.obj : $(srcdir)\filemgr.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)
    
$(objdir)\phonet.obj : $(srcdir)\phonet.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)
   
$(objdir)\hunzip.obj : $(srcdir)\hunzip.cxx
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)
    
$(objdir)\myspell_checker.obj : $(srcdir)\myspell_checker.cpp
	$(CC_OBJ) $** $(LIBENCHANT_MYSPELL_DEFINES)
    
LIBENCHANT_MYSPELL_LIB : $(LIBENCHANT_MYSPELL_OBJECTS)
	-$(RM) $(LIBENCHANT_MYSPELL_LIB)
	$(AR) $(AR_FLAGS)$(LIBENCHANT_MYSPELL_LIB) $(LIBENCHANT_MYSPELL_OBJECTS)

# libenchant_myspell.dll and libenchant_myspell.lib are created together.
LIBENCHANT_MYSPELL_DLL : $(LIBENCHANT_MYSPELL_OBJECTS)
    $(LINK_DLL) $(LIBENCHANT_MYSPELL_OBJECTS) $(LIBENCHANT_MYSPELL_LIBS) -OUT:$(LIBENCHANT_MYSPELL_DLL)
    $(EMBED_MANIFEST:exe_name=libenchant_myspell.dll)
    $(CLEAN_MANIFEST:exe_name=libenchant_myspell.dll)
    
libenchant_myspell_lib_clean:
    $(RM) $(LIBENCHANT_MYSPELL_OBJECTS) $(LIBENCHANT_MYSPELL_LIB)

libenchant_myspell_dll_clean:
    $(RM) $(LIBENCHANT_MYSPELL_OBJECTS) $(LIBENCHANT_MYSPELL_LIB) $(LIBENCHANT_MYSPELL_DLL) $(LIBENCHANT_MYSPELL_PDB)

################################################################################

check : all

mostlyclean : clean

clean: libenchant_clean libenchant_ispell_clean libenchant_myspell_clean enchant_clean enchant_lsmod_clean test-enchant_clean test-enchantxx_clean

distclean : clean

maintainer-clean : distclean

force :

