hexchat_gtk_sources = [
  'ascii.c',
  'banlist.c',
  'chanlist.c',
  'chanview.c',
  'custom-list.c',
  'dccgui.c',
  'editlist.c',
  'fe-gtk.c',
  'fkeys.c',
  'gtkutil.c',
  'ignoregui.c',
  'joind.c',
  'menu.c',
  'maingui.c',
  'notifygui.c',
  'palette.c',
  'pixmaps.c',
  'plugin-tray.c',
  'plugin-notification.c',
  'rawlog.c',
  'servlistgui.c',
  'setup.c',
  'sexy-spell-entry.c',
  'textgui.c',
  'urlgrab.c',
  'userlistgui.c',
  'xtext.c'
]

gtk_dep = dependency('gtk+-2.0', version: '>= 2.24.0')
hexchat_gtk_deps = [
  hexchat_common_dep,
  libgmodule_dep, # used by libsexy
  gtk_dep
]

if gtk_dep.get_pkgconfig_variable('target') == 'x11'
  hexchat_gtk_deps += dependency('x11')
endif

hexchat_gtk_cflags = [
  '-fPIE'
]

hexchat_gtk_ldflags = [
  '-pie'
]

if get_option('with-libnotify')
  hexchat_gtk_sources += 'notifications/notification-libnotify.c'
  hexchat_gtk_deps += dependency('libnotify')
elif false # TODO HAVE_GTK_MAC
elif host_machine.system() == 'windows'
  hexchat_gtk_sources += 'notifications/notification-windows.c'

  # TODO: mingw doesn't have these headers or libs
  # add_languages('cpp')
  # shared_module('hcnotifications-winrt',
  #  sources: 'notifications/notification-winrt.cpp'
  #)

else
  hexchat_gtk_sources += 'notifications/notification-dummy.c'
endif

iso_codes = dependency('iso-codes', required: false)
if iso_codes.found()
  hexchat_gtk_sources += 'sexy-iso-codes.c'
  iso_codes_prefix = iso_codes.get_pkgconfig_variable('prefix')
  hexchat_gtk_cflags += '-DISO_CODES_PREFIX="@0@"'.format(iso_codes_prefix)
  hexchat_gtk_cflags += '-DISO_CODES_LOCALEDIR="@0@"'.format(
                        join_paths(iso_codes_prefix, 'share/locale'))
endif

if get_option('with-plugin')
  hexchat_gtk_sources += 'plugingui.c'
endif

resources = gnome.compile_resources('resources',
  '../../data/hexchat.gresource.xml',
  source_dir: '../../data', # TODO: Fix upstream
  c_name: 'hexchat',
  extra_args: ['--manual-register']
)

executable('hexchat',
  sources:  resources + hexchat_gtk_sources,
  dependencies: hexchat_gtk_deps,
  c_args: hexchat_gtk_cflags,
  link_args: hexchat_gtk_ldflags,
  install: true,
  gui_app: true,
)
