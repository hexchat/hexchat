#!/bin/bash
PWD=$(pwd)
OS=$(uname -s)
SCRIPT_NAME=$(basename $0)
STEP=0
BASE_DIR=${PWD}
BUILD_DIR=$(dirname "${BASE_DIR}")"/build"
RELEASES_DIR=$(dirname "${BASE_DIR}")"/releases"
LOG="${BUILD_DIR}/${SCRIPT_NAME}.log"


# Build dir must be exist for the log file
rm -rf "${BUILD_DIR}" >/dev/null 2>&1
mkdir -p ${BUILD_DIR} >/dev/null 2>&1 
mkdir -p ${RELASES_DIR} >/dev/null 2>&1 


# Maybe a day this scipt can be use for debian/linux
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

  ${CMD_TEST_PKG} | grep -v "man ld" 2>/dev/null
  #ERR=$?
  ERR=${PIPESTATUS[0]}  # Get Error Code of the command and not of grep

  # Need It for gettext (OR do before run script: brew link gettext -orce)
  # And maybe OpenSSL
  # 127 -> "bin" not found (e.g. openssl version)
  # 1 -> Library not found (e.g cc -lcrypto.1.1 -Wl,--help)
  if [ ${ERR} -eq 127 ] || [ ${ERR} -eq 1 ] ; then
    BuildInstallPrefix /usr/local/opt/${PKG}
    ${CMD_TEST_PKG} | grep -v "man ld" 2>/dev/null
    #ERR=$?
    ERR=${PIPESTATUS[0]}  # Get Error Code of the command and not of grep
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
# Function PatchTextPath
################################
PatchTextPath()
{
  SRC_BINARY=$1

  SRC_BINARY_NAME=$(basename "${SRC_BINARY}")

  NB_LINE=0

  return # desactivated to see later

  strings "${SRC_BINARY}" | \
  grep -e "/etc" -e "/lib" -e "/bin" -e "/share" | \
  grep -v -e "^/etc" -e "^/lib" -e "^/bin" -e "^/share" | \
  grep -v -e "^/usr/etc" -e "^/usr/lib" -e "^/usr/bin" -e "^/usr/share" | \
  grep -v -e "^\." -e "Library" | while read LINE
    do
      if [ $NB_LINE -le 0 ] ; then
        echo "* ${SRC_BINARY}"
      fi

      echo "- ${LINE}"

      NB_LINE=$((NB_LINE+1))
    done

 
}
################################
# Function Lib2StandaloneLib
################################
Lib2StandaloneLib()
{
  SRC_BINARY=$1
  DIR_BINARIES=$2
  PREFIX=$3
  PATCH_TEXT_PATH=$4

  SRC_BINARY_NAME=$(basename "${SRC_BINARY}")

  #echo "* ${SRC_BINARY}"

  rm "${DIR_BINARIES}/ADD_LIBS" 2>/dev/null

  chmod a+w "${SRC_BINARY}"
  otool -L "${SRC_BINARY}" | grep -v -e "${SRC_BINARY_NAME}:" -e "/usr/lib/" -e "/System/Library/" | while read DYLIB DYLIB_INFO
    do
      DYLIB=${DYLIB//[[:space:]]/}
      DYLIB_NAME=$(basename $DYLIB)

      #echo "${DYLIB}"

      install_name_tool -id "${PREFIX}${DYLIB_NAME}" "${SRC_BINARY}"
      install_name_tool -change "${DYLIB}"  "${PREFIX}${DYLIB_NAME}" "${SRC_BINARY}"


      if [ ! -f "${DIR_BINARIES}/${DYLIB_NAME}" ]; then
        #echo " - $DYLIB $DYLIB_NAME"

        cp -f "${DYLIB}" "${DIR_BINARIES}/${DYLIB_NAME}"

        echo ${DIR_BINARIES}/${DYLIB_NAME} >> "${DIR_BINARIES}/ADD_LIBS"

      #  Lib2StandaloneLib "${DIR_BINARIES}/${DYLIB_NAME}" "${DIR_BINARIES}" "${PREFIX}" "${PATCH_TEXT_PATH}"

      fi
    done 

  # Experimentale patch ... by binary HACKING
  if [ "${PATCH_TEXT_PATH}" == "true" ]; then
    PatchTextPath "${SRC_BINARY}"
  fi

  chmod a-w "${SRC_BINARY}"

   # Recursivity treatment for including libs
  if [ -f "${DIR_BINARIES}/ADD_LIBS" ]; then
    IFS=$'\n' # \n is set as delimiter
    ARR_LIBS=( $(cat "${DIR_BINARIES}/ADD_LIBS") )
    IFS=' ' # space is set as delimiter

    rm "${DIR_BINARIES}/ADD_LIBS" 2>/dev/null

    #echo ${#ARR_LIBS[@]}

    for LIB in ${ARR_LIBS[@]}
    do
      #echo "LIB:${LIB}"
      Lib2StandaloneLib "${LIB}" "${DIR_BINARIES}" "${PREFIX}" "${PATCH_TEXT_PATH}"

    done
  fi

}

################################
# Function FindAndInstallBin
################################
FindAndInstallBin()
{
  SRC_BINARY=$1
  DIR_BINARIES=$2

  SRC_BINARY_NAME=$(basename "${SRC_BINARY}")


  IFS=":" # : is set as delimiter
  ARR_PATH=( $(echo "${PATH}") )
  IFS=' ' # space is set as delimiter

  for DIR_BINARY in ${ARR_PATH[@]}
  do
    FIND_BINARY="${DIR_BINARY}/${SRC_BINARY_NAME}"

    if [ -f "${FIND_BINARY}" ] ; then
      echo "- ADD: ${FIND_BINARY}"
      cp -f "${FIND_BINARY}" "${DIR_BINARIES}"
      break
    fi

  done
}

################################
# Function Main
################################
Main()
{

  Info "__        __               _             "
  Info "\ \      / /_ _ _ __ _ __ (_)_ __   __ _ "
  Info " \ \ /\ / / _\` | '__| '_ \| | '_ \ / _\` |"
  Info "  \ V  V / (_| | |  | | | | | | | | (_| |"
  Info "   \_/\_/ \__,_|_|  |_| |_|_|_| |_|\__, |"
  Info "                                   |___/ "
  Info ""
  Info "  ___  ______  __              _   "
  Info " / _ \/ ___\ \/ /  _ __   ___ | |_ "
  Info "| | | \___ \\\\  /  | '_ \ / _ \| __|"
  Info "| |_| |___) /  \  | | | | (_) | |_ "
  Info " \___/|____/_/\_\ |_| |_|\___/ \__|"
  Info ""
  Info "                                  _           _ "
  Info " ___ _   _ _ __  _ __   ___  _ __| |_ ___  __| |"
  Info "/ __| | | | '_ \| '_ \ / _ \| '__| __/ _ \/ _\` |"
  Info "\__ \ |_| | |_) | |_) | (_) | |  | ||  __/ (_| |"
  Info "|___/\__,_| .__/| .__/ \___/|_|   \__\___|\__,_|"
  Info "          |_|   |_|                             "

  Info "Hexchat did not support OSX build"
  Info "However, that is an unofficial script to make it"
  Info ""
  Info ""
  Info ""

  Info "##########################################################"
  Info "###### INSTALL DEV. ENV. AND SUPLLY LIBRARIES (ALL) ######"
  Info "##########################################################"

  # Add $HOME build prefix (For BlacDady and maybe other people)
  BuildInstallPrefix "$HOME"


  BrewCheckAndInstall openssl "openssl version"

  # Force to check a true Openssl version
  # This is : cc -lcrypto.1.1 -Wl,--help
  # it is a big hack found by me to test if the library can be find in the default search path directory
  # it is the same thing that : ld -lcrypto.1.1 --help
  # BUT ld is a fucking ....
  # --help is use to stop ld process (just check if the library exist ... OK ... On catalina and last version on Xcode this is work)
  # And i don't know why ... maybe because Xcode ld is an shitty ...
  # but i don't why ld don't look for LD_LIBRARY_PATH, LIBRARY_PATH, DYLIB_....
  # so i use xcode cc
  #
  BrewCheckAndInstall openssl@1.1 "cc -lcrypto.1.1 -Wl,--help"

  

  BrewCheckAndInstall python3 "python3 --version"
  BrewCheckAndInstall meson "meson --version"
  BrewCheckAndInstall ninja "ninja --version"

  #BrewCheckAndInstall gtk "gtk-update-icon-cache"
  BrewCheckAndInstall gtk "cc -lgdk-quartz-2.0 -Wl,--help"

  BrewCheckAndInstall pkg-config "pkg-config --version"

  #BrewCheckAndInstall libproxy "proxy --version"
  BrewCheckAndInstall libproxy "cc -lproxy -Wl,--help"

  BrewCheckAndInstall libcanberra "cc -lcanberra -Wl,--help"

  BrewCheckAndInstall dbus-glib "dbus-binding-tool --version"

  BrewCheckAndInstall libnotify "notify-send --version"

  BrewCheckAndInstall luajit "luajit -v"
  BrewCheckAndInstall gettext "gettext --version"

  BrewCheckAndInstall curl "curl --version"

  Info "#################################################"
  Info "###### MESON CONFIG / POST TREATMENT (ALL) ######"
  Info "#################################################"

  # Get Hexchat Version
  LIG_VER=$(grep "  version:" ../meson.build | tr "'" " ")
  IFS=' ' # space is set as delimiter
  read -ra ARR_VER <<< "$LIG_VER" # str is read into an array as tokens separated by IFS
  VER=${ARR_VER[1]}


  echo "###### HEXCHAT OSX Build version ######"
  meson "${BUILD_DIR}" ../ --prefix="${BUILD_DIR}/hexchat.app/Contents/Resources" --buildtype=release 


  # Change option pagezero_size on the main binary
  sed -i '' -e "s/-pagezero_size 10000//g" "${BUILD_DIR}/build.ninja"
  sed -i '' -e "s/-framework Cocoa/-framework Cocoa -Wl,-pagezero_size,0x10000/g" "${BUILD_DIR}/build.ninja"

  sed -i '' -e "s/-DHEXCHATLIBDIR=.*\/plugins\"/-DHEXCHATLIBDIR=\".\/Contents\/Resources\/lib\/hexchat\/plugins\"/g" "${BUILD_DIR}/build.ninja"

  sed -i '' -e "s/LOCALEDIR.*$/LOCALEDIR \".\/Contents\/Resources\/share\/locale\"/g" "${BUILD_DIR}/config.h"



  # TODO : TO REVIEW HEXCHAT SERVICE 
  sed -i '' -e "s/^Exec=.*$/Exec=.\/Contents\/Resources\/bin\/heschat/g" "${BUILD_DIR}/src/common/dbus/org.hexchat.service.service"
 
  # TODO : TO REVIEW IT ALSO : ADD JUST ADD TO ANOMYNOUS THE RELEASE
  sed -i '' -e "s/^prefix=.*$/prefix=.\/Contents\/Resources/g" "${BUILD_DIR}/data/pkgconfig/hexchat-plugin.pc"




  Info "###############################"
  Info "###### NINJA BUILD (ALL) ######"
  Info "###############################"

  ninja -C "${BUILD_DIR}" install


  Info "#################################################"
  Info "###### MAKE BUNDLES / POST TREATMENT (ALL) ######"
  Info "#################################################"

  mkdir -p "${BUILD_DIR}/hexchat.app/Contents/MacOS"
  mkdir -p "${BUILD_DIR}/hexchat.app/Contents/Resources/bin" >/dev/null 2>&1
  mkdir -p "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/gtk-2.0"

  ### OpenSSL Standalone Version
  mkdir -p "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/openssl/certs"
  mkdir -p "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/openssl/private"
  mkdir -p "${BUILD_DIR}/hexchat.app/Contents/Resources/lib/engines-1.1"
  cp -f "${BASE_DIR}/MozillaCACert_20200130.pem" "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/openssl/cert.pem"
  cp -f "${BASE_DIR}/certs/"* "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/openssl/certs/"

  FindAndInstallBin "openssl" "${BUILD_DIR}/hexchat.app/Contents/Resources/bin/"

  ### CURL Standalone Version
  FindAndInstallBin "curl" "${BUILD_DIR}/hexchat.app/Contents/Resources/bin/"

  ### DBUS Standalone Version
  mkdir -p "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/dbus-1/session.d"
  cp -f "${BASE_DIR}/dbus/org.freedesktop.dbus-session.plist.in" "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/dbus-1/"
  cp -f "${BASE_DIR}/dbus/session.conf.in" "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/dbus-1/"

  FindAndInstallBin "dbus-daemon" "${BUILD_DIR}/hexchat.app/Contents/Resources/bin/"

  ### OTHERS RESOURCES Standalone Version
  cp -f "${BASE_DIR}/hexchat.icns" "${BUILD_DIR}/hexchat.app/Contents/Resources/"
  cp -f "${BASE_DIR}/launcher_GIMP_model.sh" "${BUILD_DIR}/hexchat.app/Contents/MacOS/hexchat"

  #cp -f "${BASE_DIR}/gtkrc" "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/gtk-2.0/"
  cp -f "${BASE_DIR}/gtkrc_gtk+_theme_Mac" "${BUILD_DIR}/hexchat.app/Contents/Resources/etc/gtk-2.0/gtkrc"

  cp -f "${BASE_DIR}/Info.plist.in" "${BUILD_DIR}/hexchat.app/Contents/Info.plist"
  sed -i '' -e "s/@VERSION@/$VER/g" "${BUILD_DIR}/hexchat.app/Contents/Info.plist"


  # Make Portable version
  # To use the light Version, you need to Disable GateKeeper i thing because else the variable DY
  mkdir -p "${BUILD_DIR}/dylibs"
  cp -r "${BUILD_DIR}/hexchat.app" "${BUILD_DIR}/hexchat-light.app"


  # hexchat.app treatment 
  ls -ld ${BUILD_DIR}/hexchat.app/Contents/Resources/bin/* | grep '^-' | awk '{print $9}' | while read BIN_FILE
  do
    #echo "BINFILE: ${BIN_FILE}"
    Lib2StandaloneLib "${BIN_FILE}" "${BUILD_DIR}/hexchat.app/Contents/Resources/lib" "@executable_path/../lib/" "true"
  done

  ls -ld ${BUILD_DIR}/hexchat.app/Contents/Resources/lib/hexchat/plugins/*.dylib | grep '^-' | awk '{print $9}' | while read LIB_FILE
  do
    #echo "LIBFILE: ${LIB_FILE}"
    Lib2StandaloneLib "${LIB_FILE}" "${BUILD_DIR}/hexchat.app/Contents/Resources/lib" "@executable_path/../lib/" "true"
  done


  # hexchat-light.app treatment 
  ls -ld ${BUILD_DIR}/hexchat-light.app/Contents/Resources/bin/* | grep '^-' | awk '{print $9}' | while read BIN_FILE
  do
    #echo "BINFILE: ${BIN_FILE}"
    Lib2StandaloneLib "${BIN_FILE}" "${BUILD_DIR}/dylibs" "" "false"
  done

  ls -ld ${BUILD_DIR}/hexchat-light.app/Contents/Resources/lib/hexchat/plugins/*.dylib | grep '^-' | awk '{print $9}' | while read LIB_FILE
  do
    #echo "LIBFILE: ${LIB_FILE}"
    Lib2StandaloneLib "${LIB_FILE}" "${BUILD_DIR}/dylibs" "" "false"
  done



  Info "#################################"
  Info "###### MAKE RELEASES (ALL) ######"
  Info "#################################"

  # Update Releases Dir
  rm -rf "${RELEASES_DIR}/${VER}/OSX" >/dev/null 2>&1
  mkdir -p "${RELEASES_DIR}/${VER}/OSX" >/dev/null 2>&1

  cp -rf "${BUILD_DIR}/dylibs" "${RELEASES_DIR}/${VER}/OSX/"
  cp -rf "${BUILD_DIR}/hexchat.app" "${RELEASES_DIR}/${VER}/OSX/"
  cp -rf "${BUILD_DIR}/hexchat-light.app" "${RELEASES_DIR}/${VER}/OSX/"


  Info "#######################"
  Info "###### END (ALL) ######"
  Info "#######################"
  Info ""
}

################################
# MAIN BASH
################################
Main 2>&1 | tee ${LOG}
