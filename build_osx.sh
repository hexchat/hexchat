#!/bin/bash
PWD=$(pwd)
OS=$(uname -s)
SCRIPT_NAME=$(basename $0)
STEP=0
BASE_DIR=${PWD}
LOG="${BASE_DIR}/${SCRIPT_NAME}.log"

if [[ "$OS" = *[dD]arwin* ]]; then
  BREW=brew
else
  BREW=apt-get
fi


################################
# Function Info
################################
Info()
{
   echo "$(date '+%Y-%m-%d %H:%M:%S') $*"
}

################################
# Function Error
################################
Error()
{
   echo "$(date '+%Y-%m-%d %H:%M:%S') *** ERROR *** $*"
}

################################
# Function BuildInstallPrefix
################################
BuildInstallPrefix()
{
  PREF_HOME=$1

  Info "###### BuildInstallPrefix ($PREF_HOME) ######"

  # set PATH so it includes user's private bin if it exists
  if [ -d "$PREF_HOME/bin" ] ; then
      export PATH="$PREF_HOME/bin:$PATH"
  fi

  # set PATH so it includes user's private sbin if it exists
  if [ -d "$PREF_HOME/sbin" ] ; then
      export PATH="$PREF_HOME/sbin:$PATH"
  fi

  # configure var for gcc
  if [ -d "$PREF_HOME/include" ] ; then
      export C_INCLUDE_PATH="$PREF_HOME/include:$C_INCLUDE_PATH"
      export CPLUS_INCLUDE_PATH="$PREF_HOME/include:$CPLUS_INCLUDE_PATH"
      export CPATH="$PREF_HOME/include:$CPATH"
  fi

  # configure var for gcc
  if [ -d "$PREF_HOME/lib" ] ; then
      export LD_LIBRARY_PATH="$PREF_HOME/lib:$LD_LIBRARY_PATH"
      export LD_RUN_PATH="$PREF_HOME/lib:$LD_RUN_PATH"
      export LIBRARY_PATH="$PREF_HOME/lib:$LIBRARY_PATH"
  fi

  # configure var for pkg-config
  if [ -d "$PREF_HOME/lib/pkgconfig" ] ; then
      export PKG_CONFIG_PATH="$PREF_HOME/lib/pkgconfig:$PKG_CONFIG_PATH"
  fi

  Info ""

}

################################
# Function BrewCheckAndInstall
################################
BrewCheckAndInstall()
{
  PKG=$1
  CMD_TEST_PKG=$2
  CHK_ONLY=$3


  if [ $STEP -le 0 ] ; then
    Info "- HOMEBREW VERSION :"

    ${BREW} --version 2>/dev/null
    ERR=$?
    if [ ${ERR} -gt 0 ] ; then
        Error "Cannot find HOMBREW manager package (${BREW} ERROR:${ERR})"
        Info "We need to install it before run this bash"
        Info "Good Luck :D"
        exit 127
    fi

    Info ""

    STEP=$((STEP+1))
  fi


  Info "- Test Package ${PKG} :"

  ${CMD_TEST_PKG} 2>/dev/null
  ERR=$?

  # Need It for gettext (OR do before run script: brew link gettext -orce)
  # And maybe OpenSSL
  # 127 -> "bin" not found (e.g. openssl version)
  # 1 -> Library not found (e.g cc -lcrypto.1.1 -Xlinker --help)
  if [ ${ERR} -eq 127 ] || [ ${ERR} -eq 1 ] ; then
    BuildInstallPrefix /usr/local/opt/${PKG}
    ${CMD_TEST_PKG} 2>/dev/null
    ERR=$?
  fi


  if [ ${ERR} -gt 0 ] ; then

    if [ "${CHK_ONLY}" == "true" ]; then
      Error "Cannot find package ${PKG} (ERROR:${ERR})"
      Info "You are not lucky! Sorry :("
      exit 127
    else
        Info "- Install Package ${PKG} :"

        ${BREW} install ${PKG}

        BrewCheckAndInstall "${PKG}" "${CMD_TEST_PKG}" "true"
    fi
  fi

  Info ""
}


################################
# Function Main
################################
Main()
{

  Info "###### START (ALL) ######"

  # Add $HOME build prefix (For BlacDady and maybe other people)
  BuildInstallPrefix "$HOME"


  BrewCheckAndInstall openssl "openssl version"

  # Force to check a true Openssl version
  # This is : cc -lcrypto.1.1 -Xlinker --help
  # it is a big hack found by me to test if the library can be find in the default search path directory
  # it is the same thing that : ld -lcrypto.1.1 --help
  # BUT ld is a fucking ....
  # --help is use to stop ld process (just check if the library exist ... OK ... On catalina and last version on Xcode this is work)
  # And i don't know why ... maybe because Xcode ld is an shitty ...
  # but i don't why ld don't look for LD_LIBRARY_PATH, LIBRARY_PATH, DYLIB_....
  # so i use xcode cc
  #
  BrewCheckAndInstall openssl@1.1 "cc -lcrypto.1.1 -Xlinker --help"

  

  BrewCheckAndInstall python3 "python3 --version"
  BrewCheckAndInstall meson "meson --version"
  BrewCheckAndInstall ninja "ninja --version"

  #BrewCheckAndInstall gtk "gtk-update-icon-cache"
  BrewCheckAndInstall gtk "cc -lgdk-quartz-2.0 -Xlinker --help"

  BrewCheckAndInstall pkg-config "pkg-config --version"

  #BrewCheckAndInstall libproxy "proxy --version"
  BrewCheckAndInstall libproxy "cc -lproxy -Xlinker --help"

  BrewCheckAndInstall libcanberra "cc -lcanberra -Xlinker --help"

  BrewCheckAndInstall dbus-glib "dbus-binding-tool --version"

  BrewCheckAndInstall libnotify "notify-send --version"

  BrewCheckAndInstall luajit "luajit -v"
  BrewCheckAndInstall gettext "gettext --version"


  # Get Hexchat Version
  LIG_VER=$(grep "  version:" meson.build | tr "'" " ")
  IFS=' ' # space is set as delimiter
  read -ra ARR_VER <<< "$LIG_VER" # str is read into an array as tokens separated by IFS
  VER=${ARR_VER[1]}


  echo "###### HEXCHAT OSX Build version:$VER ######"
  
  rm -rf build
  meson build --prefix=${BASE_DIR}/build/hexchat.app --buildtype=release --bindir=Contents/MacOS --libdir=Contents/Resources/lib

  sed -i'' -e s/-pagezero.*-image_base/-image_base/g build/build.ninja

  ninja -C build install

  cp -f osx/hexchat.icns ${BASE_DIR}/build/hexchat.app/Contents/Resources/
  cp -f osx/Info.plist.in ${BASE_DIR}/build/hexchat.app/Contents/Info.plist
  sed -i '' -e s/@VERSION@/$VER/g ${BASE_DIR}/build/hexchat.app/Contents/Info.plist

  rm -rf ${BASE_DIR}/build/hexchat.app/include
  rm -rf ${BASE_DIR}/build/hexchat.app/share


  # Make Portable version
  mkdir ${BASE_DIR}/build/dylibs
  cp -r ${BASE_DIR}/build/hexchat.app ${BASE_DIR}/build/hexchat-portable.app

  otool -L ${BASE_DIR}/build/hexchat-portable.app/Contents/MacOS/hexchat | grep -v -e hexchat -e "/usr/lib/" | while read DYLIB DYLIB_INFO
    do
      #Remove the \t char of $DYLIB
      DYLIB=${DYLIB//[[:space:]]/}
      DYLIB_NAME=$(basename $DYLIB)
      cp -f "${DYLIB}" "${BASE_DIR}/build/dylibs/"
      cp -f "${DYLIB}" "${BASE_DIR}/build/hexchat-portable.app/Contents/Resources/lib/"

      install_name_tool -change "${DYLIB}"  "@executable_path/../Resources/lib/${DYLIB_NAME}" "${BASE_DIR}/build/hexchat-portable.app/Contents/MacOS/hexchat" 
    done




  Info "###### END (ALL) ######"
  Info ""
}

################################
# MAIN BASH
################################
Main 2>&1 | tee ${LOG}
