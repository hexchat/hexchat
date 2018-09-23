sysinfo_sources = [
  'format.c',
  'sysinfo.c',
]

sysinfo_deps = [
  libgio_dep,
  hexchat_plugin_dep,
  common_sysinfo_deps,
]

sysinfo_includes = []
sysinfo_cargs = []

system = host_machine.system()
if system == 'linux' or system == 'gnu' or system.startswith('gnu/') or system == 'darwin'
  sysinfo_includes += 'shared'
  sysinfo_sources += [
    'shared/df.c'
  ]

  if system == 'linux' or system == 'gnu' or system.startswith('gnu/')
    libpci = dependency('libpci', required: false, method: 'pkg-config')
    if libpci.found()
      sysinfo_deps += libpci
      sysinfo_cargs += '-DHAVE_LIBPCI'
      sysinfo_sources += 'unix/pci.c'

      picidsdir = libpci.get_pkgconfig_variable('idsdir')
      pciids = join_paths(picidsdir, 'pci.ids')
      sysinfo_cargs += '-DPCIIDS_FILE="@0@"'.format(pciids)
    endif
    sysinfo_includes += 'unix'
    sysinfo_sources += [
      'unix/backend.c',
      'unix/match.c',
      'unix/parse.c',
    ]
  elif system == 'darwin'
    add_languages('objc')
    sysinfo_sources += 'osx/backend.m'
  endif

elif system == 'windows'
  sysinfo_sources += [
    'win32/backend.c',
    '../../src/common/sysinfo/win32/backend.c'
  ]
else
  error('sysinfo: Unknown system?')
endif

shared_module('sysinfo', sysinfo_sources,
  dependencies: sysinfo_deps,
  include_directories: include_directories(sysinfo_includes),
  c_args: sysinfo_cargs,
  install: true,
  install_dir: plugindir,
  name_prefix: '',
)
