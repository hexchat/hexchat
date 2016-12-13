appdir = join_paths(get_option('datadir'), 'applications')

if get_option('with-gtk')
  i18n.merge_file(
    input: 'hexchat.appdata.xml.in',
    output: 'hexchat.appdata.xml',
    po_dir: '../../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'appdata')
  )

  desktop_conf = configuration_data()
  if get_option('with-dbus')
    desktop_conf.set('exec_command', 'hexchat --existing %U')
  else
    desktop_conf.set('exec_command', 'hexchat %U')
  endif

  desktop_file = configure_file(
    input: 'hexchat.desktop.in.in',
    output: 'hexchat.desktop.in',
    configuration: desktop_conf
  )

  i18n.merge_file(
    input: desktop_file,
    output: 'hexchat.desktop',
    po_dir: '../../po',
    type: 'desktop',
    install: true,
    install_dir: appdir
  )
endif

if get_option('with-theme-manager')
  i18n.merge_file(
    input: 'htm.desktop.in',
    output: 'htm.desktop',
    po_dir: '../../po',
    type: 'desktop',
    install: true,
    install_dir: appdir
  )

  install_data('htm-mime.xml',
    install_dir: join_paths(get_option('datadir'), 'mime/packages')
  )
endif
