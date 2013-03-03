#!/bin/bash

cd $(dirname $0)
autoreconf --install --symlink
if test -z "$NOCONFIGURE"; then
   exec ./configure $@
fi
