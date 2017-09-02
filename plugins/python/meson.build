python_opt = get_option('with-python')
if python_opt.startswith('python3')
  python_dep = dependency(python_opt, version: '>= 3.3')
else
  python_dep = dependency(python_opt, version: '>= 2.7')
endif

python3_source = custom_target('python-bindings',
  input: ['../../src/common/hexchat-plugin.h', 'python.py'],
  output: 'python.c',
  command: [find_program('generate_plugin.py'), '@INPUT@', '@OUTPUT@']
)

install_data(['_hexchat.py', 'hexchat.py', 'xchat.py'],
  install_dir: join_paths(get_option('libdir'), 'hexchat/python')
)

shared_module('python', python3_source,
  dependencies: [hexchat_plugin_dep, python_dep],
  install: true,
  install_dir: plugindir,
  name_prefix: '',
)
