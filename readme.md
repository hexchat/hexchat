# HexChat ReadMe

X-Chat ("xchat") Copyright (c) 1998-2010 By Peter Zelezny.
HexChat ("hexchat") Copyright (c) 2009-2013 By Berke Viktor.

This program is released under the GPL v2 with the additional exemption
that compiling, linking, and/or using OpenSSL is allowed. You may
provide binary packages linked to the OpenSSL libraries, provided that
all other requirements of the GPL are met.
See file COPYING for details.

For building instructions, see [Building](http://docs.hexchat.org/en/latest/building.html).


## What is it?

HexChat is an IRC client for Windows and UNIX operating systems. I.R.C. is
Internet Relay Chat, see [IRCHelp.org](http://irchelp.org) for information about IRC
in general. HexChat runs on most BSD and POSIX compliant operating systems.

For more information please read our documentation: http://hexchat.readthedocs.org.


## Requirements:

 * GTK+ 2.24
 * GLib 2.28

HexChat is known to work on, at least:

 * Windows Vista/7/8
 * Linux
 * FreeBSD
 * OpenBSD
 * NetBSD
 * Solaris
 * AIX
 * IRIX
 * DEC/Compaq Tru64 UNIX
 * HP-UX 10.20 and 11
 * OS X


## Python Scripts:

Consider using the Python interface for your scripts, it's a very nice
API, allows for loading/unloading individual scripts, and gives you
almost all the features of the C plugin API. For more info, see the
[HexChat Python Interface](http://hexchat.readthedocs.org/en/latest/script_python.html).


## Perl Scripts:

Perl 5.8 or newer is required. For more info, see the
[HexChat Perl Interface](http://hexchat.readthedocs.org/en/latest/script_perl.html).


## Autoloading Scripts and Plugins:

The root of your HexChat config is:

 * Windows: %APPDATA%\HexChat
 * Unix: ~/.config/hexchat

Referred to as &lt;config> from now. HexChat automatically loads, at startup:

 * &lt;config>/addons/*.pl Perl scripts
 * &lt;config>/addons/*.py Python scripts
 * &lt;config>/addons/*.dll Plugins (Windows)
 * &lt;config>/addons/*.so Plugins (Unix)
