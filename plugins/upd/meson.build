winsparkle_dep = cc.find_library('WinSparkle')

shared_module('upd', 'upd.c',
  dependencies: [winsparkle_dep, hexchat_plugin_dep],
  install: true,
  install_dir: plugindir,
  name_prefix: '',
)
