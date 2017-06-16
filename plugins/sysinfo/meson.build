sysinfo_sources = [
  'format.c',
  'sysinfo.c',
]

sysinfo_deps = [
  libgio_dep,
  hexchat_plugin_dep
]

sysinfo_includes = []

system = host_machine.system()
if system == 'linux' or system == 'darwin'
  sysinfo_includes += 'shared'
  sysinfo_sources += [
    'shared/df.c'
  ]

  if system == 'linux'
    sysinfo_deps += dependency('libpci')
    sysinfo_includes += 'unix'
    sysinfo_sources += [
      'unix/backend.c',
      'unix/match.c',
      'unix/parse.c',
      'unix/pci.c',
    ]
  elif system == 'darwin'
    add_languages('objc')
    sysinfo_sources += 'osx/backend.m'
  endif

elif system == 'windows'
  sysinfo_sources += 'win32/backend.c'
else
  error('sysinfo: Unknown system?')
endif

shared_module('sysinfo', sysinfo_sources,
  dependencies: sysinfo_deps,
  include_directories: include_directories(sysinfo_includes),
  install: true,
  install_dir: plugindir,
  name_prefix: '',
)