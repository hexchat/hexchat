#!/bin/sh

# Based on the launcher of GIMP 2.8
# Purpose: 	set up the runtime environment and run HEXCHAT

echo "Setting up environment..."

if test "x$GTK_DEBUG_LAUNCHER" != x; then
    set -x
fi

if test "x$GTK_DEBUG_GDB" != x; then
    EXEC="gdb --args"
else
    EXEC=exec
fi

name=`basename "$0"`
tmp="$0"
tmp=`dirname "$tmp"`
if [  "$tmp" == "" ] ||  [  "$tmp" == "." ] ; then                                                                              
  tmp=`pwd`
fi
tmp=`dirname "$tmp"`
bundle=`dirname "$tmp"`
bundle_contents="$bundle"/Contents
bundle_res="$bundle_contents"/Resources
bundle_lib="$bundle_res"/lib
bundle_bin="$bundle_res"/bin
bundle_data="$bundle_res"/share
bundle_etc="$bundle_res"/etc

export DYLD_LIBRARY_PATH="$bundle_lib"
export XDG_CONFIG_DIRS="$bundle_etc"/xdg
export XDG_DATA_DIRS="$bundle_data"
export GTK_DATA_PREFIX="$bundle_res"
export GTK_EXE_PREFIX="$bundle_res"
export GTK_PATH="$bundle_res"


# OPENSLL
export OPENSSL_CONF="$bundle_etc"/openssl/openssl.cnf
export CTLOG_FILE="$bundle_etc"/openssl/ct_log_list.cnf
export OPENSSL_ENGINES="$bundle_lib"/engines-1.1

if [ -f "/etc/ssl/cert.pem" ] ; then 
 # USE Apple CA Certs 
 export SSL_CERT_FILE=/etc/ssl/cert.pem
else
 # USE Mozilla CA Certs 
 export SSL_CERT_FILE="$bundle_etc"/openssl/cert.pem
fi
# Force Mozilla CA Certs 
# TODO AUTO UPDATE MOZILLA CA CERT
 export SSL_CERT_FILE="$bundle_etc"/openssl/cert.pem

export SSL_CERT_DIR="$bundle_etc"/openssl/certs

# Set up PATH variable
export PATH="$bundle_contents/MacOS:$bundle_bin:$PATH"

# Set up generic configuration
export GTK2_RC_FILES="$bundle_etc/gtk-2.0/gtkrc"
export GTK_IM_MODULE_FILE="$bundle_etc/gtk-2.0/gtk.immodules"
export GDK_PIXBUF_MODULE_FILE="$bundle_etc/gtk-2.0/gdk-pixbuf.loaders"
export GDK_PIXBUF_MODULEDIR="$bundle_lib/gdk-pixbuf-2.0/2.10.0/loaders"
#export PANGO_RC_FILE="$bundle_etc/pango/pangorc"

export PANGO_RC_FILE="$bundle_etc/pango/pangorc"
export PANGO_SYSCONFDIR="$bundle_etc"
export PANGO_LIBDIR="$bundle_lib"

# Specify Fontconfig configuration file
export FONTCONFIG_FILE="$bundle_etc/fonts/fonts.conf"

# Include GEGL path
export GEGL_PATH="$bundle_lib/gegl-0.2"


# set extra build folder
# brew without link gettext (--force)
if [ -d "/usr/local/opt/gettext/bin" ] ; then
 export PATH="/usr/local/opt/gettext/bin:$PATH"
fi

export LIBRARY_PATH="/usr/local/lib"
export DYLD_FALLBACK_LIBRARY_PATH="/usr/local/lib"

if [ -d "/usr/local/opt/gettext/lib" ] ; then
 export LIBRARY_PATH="/usr/local/opt/gettext/lib:$LIBRARY_PATH"
 export DYLD_FALLBACK_LIBRARY_PATH="/usr/local/opt/gettext/lib:$DYLD_FALLBACK_LIBRARY_PATH"
fi


# Bundle librarie folder
export LIBRARY_PATH="$bundle_lib:$LIBRARY_PATH"
export DYLD_FALLBACK_LIBRARY_PATH="$bundle_lib:$DYLD_FALLBACK_LIBRARY_PATH"


#set personal build folder prefix (at the end because should be the first using)
if [ -d "$HOME/bin" ] ; then
    export PATH="$HOME/bin:$PATH"
fi

if [ -d "$HOME/lib" ] ; then
 export LIBRARY_PATH="$HOME/lib:$LIBRARY_PATH"
 export DYLD_FALLBACK_LIBRARY_PATH="$HOME/lib:$DYLD_FALLBACK_LIBRARY_PATH"
fi

if [ -d "$HOME/lib/pkgconfig" ] ; then
 export PKG_CONFIG_PATH="$HOME/lib/pkgconfig:$PKG_CONFIG_PATH"
fi


if false; then # DOTO Include Ptyhon in the standalone version
 # Set up Python

 echo "Enabling internal Python..."
 export PYTHONHOME="$bundle_res"

 # Add bundled Python modules
 PYTHONPATH="$bundle_lib/python2.7:$PYTHONPATH"
 PYTHONPATH="$bundle_lib/python2.7/site-packages:$PYTHONPATH"
 PYTHONPATH="$bundle_lib/python2.7/site-packages/gtk-2.0:$PYTHONPATH"
 PYTHONPATH="$bundle_lib/pygtk/2.0:$PYTHONPATH"

 # Include gimp python modules
 PYTHONPATH="$bundle_lib/gimp/2.0/python:$PYTHONPATH"
 export PYTHONPATH
fi

# Set custom Poppler Data Directory
export POPPLER_DATADIR="$bundle_data/poppler"

# Specify Ghostscript directories
# export GS_RESOURCE_DIR="$bundle_res/share/ghostscript/9.06/Resource"
# export GS_ICC_PROFILES="$bundle_res/share/ghostscript/9.06/iccprofiles/"
# export GS_LIB="$GS_RESOURCE_DIR/Init:$GS_RESOURCE_DIR:$GS_RESOURCE_DIR/Font:$bundle_res/share/ghostscript/fonts:$bundle_res/share/fonts/urw-fonts:$GS_ICC_PROFILES"
# export GS_FONTPATH="$bundle_res/share/ghostscript/fonts:$bundle_res/share/fonts/urw-fonts:~/Library/Fonts:/Library/Fonts:/System/Library/Fonts"

# set up character encoding aliases
if test -f "$bundle_lib/charset.alias"; then
 export CHARSETALIASDIR="$bundle_lib"
fi

# Extra arguments can be added in environment.sh.
EXTRA_ARGS=
if test -f "$bundle_res/environment.sh"; then
 source "$bundle_res/environment.sh"
fi

# Strip out the argument added by the OS.
if /bin/expr "x$1" : '^x-psn_' > /dev/null; then
 shift 1
fi

echo "Launching HEXCHAT..."
cd "$bundle"
#$EXEC "$bundle_contents/MacOS/$name-bin" "$@" $EXTRA_ARGS
$EXEC "$bundle_bin/$name" "$@" $EXTRA_ARGS
