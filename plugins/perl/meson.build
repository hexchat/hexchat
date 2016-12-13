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

irc_perl_module = custom_target('irc-perl-header',
  input: 'lib/IRC.pm',
  output: 'irc.pm.h',
  command: [generate_perl_header, '@OUTPUT@', '@INPUT@']
)

perl = find_program('perl')

ret = run_command([perl, '-MExtUtils::Embed', '-e', 'ccopts'])
if ret.returncode() != 0
  error('perl: Failed to get cflags')
endif
perl_cflags = []
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
foreach flag : ret.stdout().strip().split(' ')
  if flag.startswith('-L') or flag.startswith('-l')
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

shared_module('perl',
  sources: ['perl.c', hexchat_perl_module, irc_perl_module],
  dependencies: [libgio_dep, hexchat_plugin_dep],
  c_args: perl_cflags,
  link_args: perl_ldflags,
  install: true,
  install_dir: plugindir,
  name_prefix: '',
)
