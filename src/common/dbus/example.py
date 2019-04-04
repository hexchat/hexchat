#!/usr/bin/python

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

from gi.repository import Gio

bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
connection = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
  						'org.hexchat.service', '/org/hexchat/Remote', 'org.hexchat.connection', None)
path = connection.Connect('(ssss)', 
					'example.py',
					'Python example', 
					'Example of a D-Bus client written in python', 
					'1.0')		
hexchat = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
								'org.hexchat.service', path, 'org.hexchat.plugin', None)
         
# Note the type before every arguement, this must be done.
# Type requirements are listed in our docs and characters are listed in the dbus docs.
# s = string, u = uint, i = int, etc.

channels = hexchat.ListGet ('(s)', "channels")
while hexchat.ListNext ('(u)', channels):
	name = hexchat.ListStr ('(us)', channels, "channel")
	print("------- " + name + " -------")
	hexchat.SetContext ('(u)', hexchat.ListInt ('(us)', channels, "context"))
	hexchat.EmitPrint ('(sas)', "Channel Message", ["John", "Hi there", "@"])
	users = hexchat.ListGet ('(s)', "users")
	while hexchat.ListNext ('(u)', users):
		print("Nick: " + hexchat.ListStr ('(us)', users, "nick"))
	hexchat.ListFree ('(u)', users)
hexchat.ListFree ('(u)', channels)

print(hexchat.Strip ('(sii)', "\00312Blue\003 \002Bold!\002", -1, 1|2))
