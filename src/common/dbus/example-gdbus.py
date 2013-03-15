#!/usr/bin/python

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
