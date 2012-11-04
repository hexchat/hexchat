# HexChat ReadMe

X-Chat ("xchat") Copyright (c) 1998-2010 By Peter Zelezny.  
HexChat ("hexchat") Copyright (c) 2009-2012 By Berke Viktor.

This program is released under the GPL v2 with the additional exemption
that compiling, linking, and/or using OpenSSL is allowed. You may
provide binary packages linked to the OpenSSL libraries, provided that
all other requirements of the GPL are met.
See file COPYING for details.

For building instructions, see [Building](http://www.hexchat.org/developers/building).

## What is it?

HexChat is an IRC client for Windows and UNIX operating systems. I.R.C. is
Internet Relay Chat, see [IRCHelp.org](http://irchelp.org) for more information about IRC
in general. HexChat runs on most BSD and POSIX compliant operating systems.

## Requirements:

 * GTK+ 2.10
 * GLib 2.12

HexChat is known to work on, at least:

 * Windows XP/Vista/7/8
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


## Notes for packagers:

If you need your packages to work on i386, you don't need to compile with
--disable-mmx, because it's also checked at run-time.

## Python Scripts:

Scripts for XChat 1.8.x are not compatible, a brand new interface has been written. For more info, see the [HexChat Python Interface](https://github.com/hexchat/hexchat/blob/master/share/doc/python.md).

Consider using the Python interface for your scripts, it's a very nice
API, allows for loading/unloading individual scripts, and gives you
almost all the features of the C-Plugin API.


## Perl Scripts:

Perl 5.8 or newer is required. For more info, see the [HexChat Perl Interface](https://github.com/hexchat/hexchat/blob/master/share/doc/perl.md).

Scripts for XChat 1.8.x are compatible with the following exceptions:

 * IRC::command will not interpret %C, %B, %U etc.

 * user_list and user_list_short:
  If a user has both op and voice, only the op flag will be 1.

 * add_user_list/sub_user_list/clear_user_list
  These functions do nothing.

 * notify_list
  Not implemented. Always returns an empty list.

 * server_list
  Lists servers that are not connected aswell.

 * Some print events may have new names and some were added.

 * Text printed by scripts must now be UTF8.

 * Text passed to scripts (via add_message_handler) will be encoded in UTF8.

## Autoloading Scripts and Plugins:

The root of your HexChat config is:

 * Windows: %APPDATA%\HexChat
 * Unix: ~/.config/hexchat

Referred to as &lt;config> from now. HexChat automatically loads, at startup:

 * &lt;config>/addons/*.lua Lua scripts
 * &lt;config>/addons/*.pl Perl scripts
 * &lt;config>/addons/*.py Python scripts
 * &lt;config>/addons/*.tcl Tcl scripts
 * &lt;config>/addons/*.dll Plugins (Windows)
 * &lt;config>/addons/*.so Plugins (Unix)

## Control Codes:

 * %%     -  A single percentage sign
 * %C     -  Control-C (mIRC color code)
 * %B     -  Bold Text
 * %U     -  Underline Text
 * %R     -  Reverse Text
 * %O     -  Reset all Text attributes
 * %XXX   -  ASCII XXX (where XXX is a decimal 3 digit number, e.g.: %007 sends a BEEP)

%Cforeground,background will produce a color code, e.g.: %C03,10

These are now disabled by default (see _Settings_ `->` _Preferences_ `->` _Input Box_).  
Instead you can insert the real codes via ctrl-k, ctrl-b and ctrl-o.
