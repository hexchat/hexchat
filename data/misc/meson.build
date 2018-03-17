appdir = join_paths(get_option('datadir'), 'applications')
metainfodir = join_paths(get_option('datadir'), 'metainfo')
desktop_utils = find_program('desktop-file-validate', required: false)

if get_option('with-gtk')
  hexchat_appdata = i18n.merge_file(
    input: 'io.github.Hexchat.appdata.xml.in',
    output: 'io.github.Hexchat.appdata.xml',
    po_dir: '../../po',
    install: true,
    install_dir: metainfodir
  )

  appstream_util = find_program('appstream-util', required: false)
  if appstream_util.found()
    test('Validate io.github.Hexchat.appdata.xml', appstream_util,
      args: ['validate-relax', hexchat_appdata]
    )
  endif

  desktop_conf = configuration_data()
  if get_option('with-dbus')
    desktop_conf.set('exec_command', 'hexchat --existing %U')
  else
    desktop_conf.set('exec_command', 'hexchat %U')
  endif

  desktop_file = configure_file(
    input: 'io.github.Hexchat.desktop.in.in',
    output: 'io.github.Hexchat.desktop.in',
    configuration: desktop_conf
  )

  hexchat_desktop = i18n.merge_file(
    input: desktop_file,
    output: 'io.github.Hexchat.desktop',
    po_dir: '../../po',
    type: 'desktop',
    install: true,
    install_dir: appdir
  )

  if desktop_utils.found()
    test('Validate io.github.Hexchat.desktop', desktop_utils,
      args: [hexchat_desktop]
    )
  endif
endif

if get_option('with-theme-manager')
  htm_desktop = i18n.merge_file(
    input: 'io.github.Hexchat.ThemeManager.desktop.in',
    output: 'io.github.Hexchat.ThemeManager.desktop',
    po_dir: '../../po',
    type: 'desktop',
    install: true,
    install_dir: appdir
  )

  if desktop_utils.found()
    test('Validate io.github.Hexchat.ThemeManager.desktop', desktop_utils,
      args: [htm_desktop]
    )
  endif

  install_data('io.github.Hexchat.ThemeManager.xml',
    install_dir: join_paths(get_option('datadir'), 'mime/packages')
  )
endif

if get_option('with-plugin')
  plugin_metainfo = []

  # FIXME: These should all get translated somewhere
  if get_option('with-checksum')
    plugin_metainfo += [
      ['Checksum', 'Calculates a checksum for all sent and recieved DCC files']
    ]
  endif

  if get_option('with-fishlim')
    plugin_metainfo += [
      ['Fishlim', 'Allows setting a key for encrypted conversations']
    ]
  endif

  if get_option('with-lua') != 'false'
    plugin_metainfo += [
      ['Lua', 'Provides a scripting interface in Lua']
    ]
  endif

  if get_option('with-perl')
    plugin_metainfo += [
      ['Perl', 'Provides a scripting interface in Perl']
    ]
  endif

  if get_option('with-python') != 'false'
    plugin_metainfo += [
      ['Python', 'Provides a scripting interface in Python']
    ]
  endif

  if get_option('with-sysinfo')
    plugin_metainfo += [
      ['Sysinfo', 'Adds command to display system information']
    ]
  endif

  foreach metainfo : plugin_metainfo
    name = metainfo[0]
    desc = metainfo[1]
    conf = configuration_data()
    conf.set('NAME', name)
    conf.set('SUMMARY', desc)

    configure_file(
      input: 'io.github.Hexchat.Plugin.metainfo.xml.in',
      output: 'io.github.Hexchat.Plugin.@0@.metainfo.xml'.format(name),
      configuration: conf,
      install_dir: get_option('install-plugin-metainfo') ? metainfodir : '',
    )
  endforeach
endif