python_opt = get_option('with-python')
if python_opt.startswith('python3')
  # Python 3.8 introduced a new -embed variant
  if not python_opt.endswith('-embed')
    python_dep = dependency(python_opt + '-embed', version: '>= 3.3', required: false)
    if not python_dep.found()
      python_dep = dependency(python_opt, version: '>= 3.3')
    endif
  else
    python_dep = dependency(python_opt, version: '>= 3.3')
  endif
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
  vs_module_defs: 'python.def'
)
