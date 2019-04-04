if cc.get_id() == 'msvc'
  lua_dep = cc.find_library('lua51')
else
  lua_dep = dependency(get_option('with-lua'))
endif

shared_module('lua', 'lua.c',
  dependencies: [libgio_dep, hexchat_plugin_dep, lua_dep],
  install: true,
  install_dir: plugindir,
  name_prefix: '',
)
