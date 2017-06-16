add_languages('cs')

executable('thememan',
  sources: [
    'Program.cs',
    'Main.cs',
    'Main.Designer.cs',
  ],
  resources: [
    # 'Main.resx', # FIXME: Allow setting resource id
    'thememan.HTM.resources',
  ],
  cs_args: [
    '-r:WindowsBase',
    '-r:System.Data',
    '-r:System.Drawing',
    '-r:System.Windows.Forms',
    '-win32icon:' + meson.current_source_dir() + '/Resources/htm.ico'
  ],
  install: true,
)

thememan_conf = configuration_data()
thememan_conf.set('exec_prefix', get_option('prefix'))
thememan_conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

configure_file(
  input: 'thememan.in',
  output: 'thememan',
  configuration: thememan_conf,
  install: true,
  install_dir: get_option('bindir'),
)
