#! /usr/bin/python

import dbus

bus = dbus.SessionBus()
proxy_obj = bus.get_object('org.xchat.service', '/org/xchat/Manager')
manager = dbus.Interface(proxy_obj, 'org.xchat.manager')
path = manager.Connect ()
proxy_obj = bus.get_object('org.xchat.service', path)
xchat = dbus.Interface(proxy_obj, 'org.xchat.remote')

channels = xchat.ListGet ("channels")
while xchat.ListNext (channels):
	name = xchat.ListStr (channels, "channel")
	print "------- " + name + " -------"
	xchat.SetContext (xchat.ListInt (channels, "context"))
	xchat.EmitPrint ("Channel Message", ["John", "Hi there", "@"])
	users = xchat.ListGet ("users")
	while xchat.ListNext (users):
		print "Nick: " + xchat.ListStr (users, "nick")
	xchat.ListFree (users)
xchat.ListFree (channels)

print xchat.Strip ("\00312Blue\003 \002Bold!\002", -1, 1|2)
