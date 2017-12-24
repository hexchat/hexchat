appdir = join_paths(get_option('datadir'), 'applications')
desktop_utils = find_program('desktop-file-validate', required: false)

if get_option('with-gtk')
  hexchat_appdata = i18n.merge_file(
    input: 'io.github.Hexchat.appdata.xml.in',
    output: 'io.github.Hexchat.appdata.xml',
    po_dir: '../../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'appdata')
  )

  appstream_util = find_program('appstream-util', required: false)
  if appstream_util.found()
    test('Validate io.github.Hexchat.appdata.xml', appstream_util,
      args: ['validate', hexchat_appdata]
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
