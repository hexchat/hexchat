plugindir = join_paths(get_option('libdir'), 'hexchat/plugins')

if host_machine.system() == 'windows'
  if get_option('with-exec')
    subdir('exec')
  endif

  if get_option('with-upd')
    subdir('upd')
  endif

  if get_option('with-winamp')
    subdir('winamp')
  endif
endif

if get_option('with-checksum')
  subdir('checksum')
endif

if get_option('with-fishlim')
  subdir('fishlim')
endif

if get_option('with-lua') != 'false'
  subdir('lua')
endif

if get_option('with-perl') != 'false'
  subdir('perl')
endif

if get_option('with-python') != 'false'
  subdir('python')
endif

if get_option('with-sysinfo')
  subdir('sysinfo')
endif
