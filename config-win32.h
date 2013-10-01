#define LOCALEDIR ".\\share\\locale"
#define ENABLE_NLS
#define USE_GMODULE
#define USE_PLUGIN
#define USE_OPENSSL
#define USE_IPV6
#define HAVE_ISO_CODES
#define ISO_CODES_PREFIX ".\\"
#define ISO_CODES_LOCALEDIR LOCALEDIR
#define PACKAGE_NAME "hexchat"
#define PACKAGE_VERSION "2.9.6"
#define HEXCHATLIBDIR ".\\plugins"
#define HEXCHATSHAREDIR "."
#define OLD_PERL
#define GETTEXT_PACKAGE "hexchat"
#define PACKAGE_TARNAME "hexchat-2.9.6"
#ifndef USE_IPV6
#define socklen_t int
#endif
