#!/usr/bin/env python3

import os
import sys
import subprocess

prefix = os.environ.get('MESON_INSTALL_PREFIX', '/usr/local')
datadir = os.path.join(prefix, 'share')
with_thememan = sys.argv[1] == 'true'

# Packaging tools define DESTDIR and this isn't needed for them
if 'DESTDIR' not in os.environ:
    print('Updating icon cache...')
    subprocess.call(['gtk-update-icon-cache', '-qtf',
                     os.path.join(datadir, 'icons', 'hicolor')])

    print('Updating desktop database...')
    subprocess.call(['update-desktop-database', '-q',
                     os.path.join(datadir, 'applications')])

    if with_thememan:
        print('Updating mime database...')
        subprocess.call(['update-mime-database',
                         os.path.join(datadir, 'mime')])
