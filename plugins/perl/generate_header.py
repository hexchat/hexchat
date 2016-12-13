#!/usr/bin/env python3

import sys
from os.path import basename

out_file = sys.argv[1]
in_files = sys.argv[2:]


def escape_perl(file):
    ret = ''
    for line in file:
        # Escape " and \, strip empty space, shove in C strings.
        ret += '"' + line.strip().replace('\\', '\\\\').replace('"', '\\"') + '\\n"\n'
    return ret


with open(out_file, 'w') as o:
    o.write('"BEGIN {\\n"\n')
    for in_file in in_files:
        o.write("\"$INC{{'{}'}} = 'Compiled into the plugin.';\\n\"\n".format(basename(in_file)))
    o.write('"}\\n"\n')

    for in_file in in_files:
        o.write('"{\\n"\n')
        o.write('"#line 1 \\"{}\\"\\n"\n'.format(basename(in_file)))
        with open(in_file) as i:
            o.write(escape_perl(i))
        o.write('"}\\n"\n')
