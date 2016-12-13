project('hexchat', 'c',
  version: '2.12.4',
  meson_version: '>= 0.37.0',
  default_options: [
    'c_std=gnu89',
    'buildtype=debugoptimized',
    'warning_level=1',
  ]
)

i18n = import('i18n')
gnome = import('gnome')
cc = meson.get_compiler('c')


libgio_dep = dependency('gio-2.0', version: '>= 2.34.0')
libgmodule_dep = dependency('gmodule-2.0')
if cc.get_id() == 'msvc'
  libssl_dep = cc.find_library('libeay32')
else
  libssl_dep = dependency('openssl', version: '>= 0.9.8',
                          required: get_option('with-ssl'))
endif

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('GETTEXT_PACKAGE', 'hexchat')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'),
                                 get_option('datadir'), 'locale'))
config_h.set10('ENABLE_NLS', true)

# Optional features
config_h.set('USE_OPENSSL', get_option('with-ssl'))
config_h.set('USE_LIBPROXY', get_option('with-libproxy'))
config_h.set('USE_LIBCANBERRA', get_option('with-libcanberra'))
config_h.set('USE_DBUS', get_option('with-dbus'))
config_h.set('USE_PLUGIN', get_option('with-plugin'))

config_h.set('G_DISABLE_SINGLE_INCLUDES', true)
config_h.set('GTK_DISABLE_DEPRECATED', true)
config_h.set('GTK_DISABLE_SINGLE_INCLUDES', true)
config_h.set('GDK_PIXBUF_DISABLE_SINGLE_INCLUDES', true)
config_h.set('GLIB_VERSION_MAX_ALLOWED', 'GLIB_VERSION_2_34')
config_h.set('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_2_34')

# Detected features
config_h.set('HAVE_MEMRCHR', cc.has_function('memrchr'))
config_h.set('HAVE_STRINGS_H', cc.has_header('strings.h'))

if libssl_dep.found()
  config_h.set('HAVE_X509_GET_SIGNATURE_NID',
    cc.has_function('X509_get_signature_nid', dependencies: libssl_dep)
  )
  config_h.set('HAVE_SSL_CTX_GET_SSL_METHOD',
    cc.has_function('SSL_CTX_get_ssl_method', dependencies: libssl_dep)
  )
  config_h.set('HAVE_DH_SET0_PQG',
    cc.has_function('DH_set0_pqg', dependencies: libssl_dep)
  )
  config_h.set('HAVE_DH_GET0_KEY',
    cc.has_function('DH_get0_key', dependencies: libssl_dep)
  )
  config_h.set('HAVE_DH_SET0_KEY',
    cc.has_function('DH_set0_key', dependencies: libssl_dep)
  )
endif

configure_file(output: 'config.h', configuration: config_h)
config_h_include = include_directories('.')

if host_machine.system() == 'windows'
  add_project_arguments('-DWIN32', language: 'c')
endif


global_cflags = []
test_cflags = [
  '-pipe',
  '-funsigned-char',
  '-Wno-conversion',
  '-Wno-pointer-sign',
  '-Wno-padded',
  '-Wno-unused-parameter',
  '-Wno-missing-prototypes',
  '-Winline',
  '-Wstrict-prototypes',
  '-Werror=implicit-function-declaration',
  '-Werror=pointer-arith',
  '-Werror=init-self',
  ['-Werror=format-security', '-Werror=format=1'],
  '-Werror=missing-include-dirs',
  '-Werror=date-time',
]
if host_machine.system() != 'windows' and get_option('buildtype') != 'plain'
  test_cflags += '-fstack-protector-strong'
endif
foreach cflag : test_cflags
  if cc.has_multi_arguments(cflag)
    global_cflags += cflag
  endif
endforeach
add_project_arguments(global_cflags, language: 'c')

if host_machine.system() != 'windows'
  global_ldflags = []
  test_ldflags = [
    '-Wl,-z,relro',
    '-Wl,-z,now',
  ]
  foreach ldflag : test_ldflags
    if cc.has_argument(ldflag)
      global_ldflags += ldflag
    endif
  endforeach
  add_project_link_arguments(global_ldflags, language: 'c')
endif

subdir('src')
if get_option('with-plugin')
  subdir('plugins')
endif
if cc.get_id() != 'msvc'
  subdir('data')
  subdir('po') # FIXME: build xgettext

  meson.add_install_script('meson_post_install.py',
    '@0@'.format(get_option('with-theme-manager'))
  )
endif
