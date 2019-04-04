generate_perl_header = find_program('generate_header.py')

hexchat_perl_module = custom_target('hexchat-perl-header',
  input: [
    'lib/HexChat.pm',
    'lib/Xchat.pm',
    'lib/HexChat/Embed.pm',
    'lib/HexChat/List/Network.pm',
    'lib/HexChat/List/Network/Entry.pm',
    'lib/HexChat/List/Network/AutoJoin.pm',
  ],
  output: 'hexchat.pm.h',
  command: [generate_perl_header, '@OUTPUT@', '@INPUT@']
)

perl_cflags = []
irc_perl_module = []

if get_option('with-perl-legacy-api')
  irc_perl_module = custom_target('irc-perl-header',
    input: 'lib/IRC.pm',
    output: 'irc.pm.h',
    command: [generate_perl_header, '@OUTPUT@', '@INPUT@']
  )
  perl_cflags += '-DOLD_PERL'
endif

perl = find_program(get_option('with-perl'))

ret = run_command([perl, '-MExtUtils::Embed', '-e', 'ccopts'])
if ret.returncode() != 0
  error('perl: Failed to get cflags')
endif
foreach flag : ret.stdout().strip().split(' ')
  if flag.startswith('-I') or flag.startswith('-D')
    perl_cflags += flag
  endif
endforeach

ret = run_command([perl, '-MExtUtils::Embed', '-e', 'ldopts'])
if ret.returncode() != 0
  error('perl: Failed to get ldflags')
endif
perl_ldflags = []
perl_rpath = ''
foreach flag : ret.stdout().strip().split(' ')
  if flag.startswith('-L') or flag.startswith('-l')
    perl_ldflags += flag
  endif
  if flag.startswith('-Wl,-rpath,')
    # Install rpath
    split = flag.split(',')
    perl_rpath = split[split.length() - 1]

    # For in tree
    perl_ldflags += flag
  endif
endforeach

perl_cflags += [
  # Perl has its own 'config.h' that we must override
  # TODO: Just rename ours to something more unique.
  '-include', meson.build_root() + '/config.h'
]

if not cc.links('''
#define PERL_NO_INLINE_FUNCTIONS
#include <EXTERN.h>
#include <perl.h>

int main(void) {
  PerlInterpreter *my_perl = perl_alloc();
  return 0;
}
''', args: perl_cflags + perl_ldflags)
  error('found perl not suitable for plugin')
endif

perl_dep = declare_dependency(
  compile_args: perl_cflags,
  link_args: perl_ldflags
)

shared_module('perl',
  sources: ['perl.c', hexchat_perl_module, irc_perl_module],
  dependencies: [libgio_dep, hexchat_plugin_dep, perl_dep],
  install: true,
  install_dir: plugindir,
  install_rpath: perl_rpath,
  name_prefix: '',
)
