#! /usr/bin/python

# HexChat
# Copyright (C) 1998-2010 Peter Zelezny.
# Copyright (C) 2009-2013 Berke Viktor.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#

import dbus

bus = dbus.SessionBus()
proxy = bus.get_object('org.hexchat.service', '/org/hexchat/Remote')
remote = dbus.Interface(proxy, 'org.hexchat.connection')
path = remote.Connect ("example.py",
		       "Python example",
		       "Example of a D-Bus client written in python",
		       "1.0")
proxy = bus.get_object('org.hexchat.service', path)
hexchat = dbus.Interface(proxy, 'org.hexchat.plugin')

channels = hexchat.ListGet ("channels")
while hexchat.ListNext (channels):
	name = hexchat.ListStr (channels, "channel")
	print("------- " + name + " -------")
	hexchat.SetContext (hexchat.ListInt (channels, "context"))
	hexchat.EmitPrint ("Channel Message", ["John", "Hi there", "@"])
	users = hexchat.ListGet ("users")
	while hexchat.ListNext (users):
		print("Nick: " + hexchat.ListStr (users, "nick"))
	hexchat.ListFree (users)
hexchat.ListFree (channels)

print(hexchat.Strip ("\00312Blue\003 \002Bold!\002", -1, 1|2))

