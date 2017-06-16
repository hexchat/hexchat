python_opt = get_option('with-python')
if python_opt.startswith('python3')
  python_dep = dependency(python_opt, version: '>= 3.3')
else
  python_dep = dependency(python_opt, version: '>= 2.7')
endif

shared_module('python', 'python.c',
  dependencies: [libgio_dep, hexchat_plugin_dep, python_dep],
  install: true,
  install_dir: plugindir,
  name_prefix: '',
)
