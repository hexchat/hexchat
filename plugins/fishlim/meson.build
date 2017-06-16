if not libssl_dep.found()
  error('fish plugin requires openssl')
endif

fishlim_sources = [
  'dh1080.c',
  'fish.c',
  'irc.c',
  'keystore.c',
  'plugin_hexchat.c'
]

shared_module('fishlim', fishlim_sources,
  dependencies: [libgio_dep, hexchat_plugin_dep, libssl_dep],
  install: true,
  install_dir: plugindir,
  name_prefix: '',
)
