man_conf = configuration_data()
man_conf.set('VERSION', meson.project_version())

configure_file(
  input: 'hexchat.1.in',
  output: 'hexchat.1',
  configuration: man_conf,
  install: true,
  install_dir: join_paths(get_option('mandir'), 'man1')
)
