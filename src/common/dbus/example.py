#! /usr/bin/python

import dbus

bus = dbus.SessionBus()
proxy_obj = bus.get_object('org.xchat.service', '/org/xchat/Manager')
manager = dbus.Interface(proxy_obj, 'org.xchat.manager')
path = manager.Connect ()
proxy_obj = bus.get_object('org.xchat.service', path)
xchat = dbus.Interface(proxy_obj, 'org.xchat.remote')

context = xchat.FindContext ("", "#test")
xchat.SetContext (context)
lst = xchat.ListGet ("users")
while xchat.ListNext (lst):
	print xchat.ListStr (lst, "nick")
xchat.ListFree (lst)

manager.Disconnect()
