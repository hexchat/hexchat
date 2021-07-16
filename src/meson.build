subdir('common')

if get_option('gtk-frontend')
  subdir('fe-gtk')

  if host_machine.system() == 'windows'
    subdir('libenchant_win8')
  endif
endif

if get_option('text-frontend')
  subdir('fe-text')
endif

if get_option('theme-manager')
  subdir('htm')
endif
