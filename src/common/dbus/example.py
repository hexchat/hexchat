#! /usr/bin/python

import dbus

bus = dbus.SessionBus()
proxy_obj = bus.get_object('org.xchat.service', '/org/xchat/RemoteObject')
xchat = dbus.Interface(proxy_obj, 'org.xchat.interface')

lst = xchat.ListGet ("users")
while xchat.ListNext (lst):
	print xchat.ListStr (lst, "nick")
xchat.ListFree (lst)
