#! /usr/bin/python

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

