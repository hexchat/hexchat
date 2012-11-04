# HexChat Python Interface

## Features

Here are some of the features of the python plugin interface:

 * Comprehensive, consistent and straightforward API
 * Load, unload, reload, and autoload support
 * Per plugin independent interpreter state
 * Python interactive console
 * Python interactive command execution
 * Full thread support
 * Stdout and stderr redirected to HexChat console
 * Dynamic list management
 * Nice context treatment
 * Plugin preferences

## Commands

The following commands will be intercepted by the Python Plugin interface module, when it is loaded.

 * **/py load &lt;filename>:** Load module with given filename.
 * **/py unload &lt;filename|module name>:** Unload module with given filename, or module name.
 * **/py reload &lt;filename|module name>:** Reload module with given filename, or module name.
 * **/py list:** List Python modules loaded.
 * **/py exec &lt;command>:** Execute given Python command interactively. For example:

<pre>
	/py exec import xchat
	/py exec print xchat.get_info('channel')
</pre>

 * **/py console:** Open the Python interactive console in a query (>>python<<).
Every message sent will be intercepted by the Python plugin interface,
and interpreted interactively. Notice that the console and /py exec
commands live in the same interpreter state.

 * **/py about:** Show some information about the Python plugin interface.


## Autoloading modules

If you want some module to be autoloaded together with the Python plugin
interface (which usually loads at startup time), just make sure it has a
`.py` extension and put it in your HexChat directory (`~/.config/hexchat/addons`, `%APPDATA%\\HexChat\\addons`).


## Context theory

Before starting to explain what the API offers, I'll do a short
introduction about the HexChat context concept. Not because it's something
hard to understand, but because you'll understand better the API
explanations if you know what I'm talking about.

You can think about a context as an HexChat channel, server, or query tab.
Each of these tabs, has its own context, and is related to a given
server and channel (queries are a special kind of channel).

The *current* context is the one where HexChat passes control to the
module. For example, when HexChat receives a command in a specific
channel, and you have asked HexChat to tell you about this event, the
current context will be set to this channel before your module is
called.


## Hello world

Here is the traditional _hello world_ example.

<pre>
__module_name__ = "helloworld"
__module_version__ = "1.0"
__module_description__ = "Python module example"

print "Hello world!"
</pre>

This module will print "Hello world!" in the HexChat console, and sleep
forever until it's unloaded. It's a simple module, but already
introduces some concepts. Notice how the module information is set. This
information is obligatory, and will be shown when listing the loaded
HexChat modules.


## xchat module

The xchat module is your passport to every HexChat functionality offered
by the Python plugin interface. Here's a simple example:

<pre>
import xchat
xchat.prnt("Hi everyone!")
</pre>

The following functions are available in the xchat module.


### Generic functions


#### xchat.prnt(string)

This function will print string in the current context. It's mainly
useful as a parameter to pass to some other function, since the usual
print statement will have the same results. You have a usage example
above.

This function is badly named because `"print"` is a reserved keyword of the Python language.


#### xchat.emit\_print(event\_name, \*args)

This function will generate a *print event* with the given arguments. To
check which events are available, and the number and meaning of
arguments, have a look at the `Settings > Lists > Text Events` window.
Here is one example:

<pre>
xchat.emit_print("Channel Message", "John", "Hi there", "@")
</pre>


#### xchat.command(string)

Execute the given command in the current context. This has the same
results as executing a command in the HexChat window, but notice that the
`/` prefix is not used. Here is an example:

<pre>
xchat.command("server irc.openprojects.net")
</pre>


#### xchat.nickcmp(s1, s2)

This function will do an RFC1459 compliant string comparing between `s1`
and `s2`, and is useful to compare channels and nicknames. It returns an
integer less than, equal to, or greater than zero if `s1` is found,
respectively, to be less than, to match, or be greater than `s2`. For
example:

<pre>
if xchat.nickcmp(nick, "mynick") == 0:
    print "They are the same!"
</pre>


### Information retreiving functions


#### xchat.get\_info(type)

Retrieve the information specified by the `type` string in the current
context. At the moment of this writing, the following information types
are available to be queried:

 * **away:** Away reason or None if you are not away.
 * **channel:** Channel name of the current context.
 * **charset:** Charset in current context.
 * **configdir:** HexChat config directory e.g.: "~/.config/hexchat".
 * **event_name NAME:** Returns text event string for requested event.
 * **gtkwin_ptr:** Current Gtk Window.
 * **host:** Real hostname of the server you connected to.
 * **inputbox:** Contents of inputbox.
 * **network:** Current network name or None.
 * **nick:** Your current nick name.
 * **nickserv:** Current networks nickserv password or None.
 * **modes:** Current channel modes or None.
 * **server:** Current server name (what the server claims to be) or None if you are not connected.
 * **topic:** Current channel topic.
 * **win_status:** Returns status of window: 'active', 'hidden', or 'normal'.
 * **version:** HexChat version number.

Example:

<pre>
if xchat.get_info("server") is None:
    print "Not connected!"
</pre>


#### xchat.get\_prefs(name)

Retrieve the HexChat setting information specified by the `name` string,
as available by the `/set` command. For example:

<pre>
print "Current preferred nick:", xchat.get_prefs("irc_nick1")
</pre>


#### xchat.get\_list(type)

With this function you may retrieve a list containing the selected
information from the current context, like a DCC list, a channel list, a
user list, etc. Each list item will have its attributes set dynamically
depending on the information provided by the list type.

The example below is a rewrite of the example provided with HexChat's
plugin API documentation. It prints a list of every DCC transfer
happening at the moment. Notice how similar the interface is to the C
API provided by HexChat.

<pre>
list = xchat.get_list("dcc")
if list:
    print "--- DCC LIST ------------------"
    print "File  To/From   KB/s   Position"
    for i in list:
        print "%6s %10s %.2f  %d" % (i.file, i.nick, i.cps/1024, i.pos)
</pre>

Below you will find what each list type has to offer.

This information was taken from HexChat's plugin documentation. You may find any types not
listed here, if they exist at all, in an updated HexChat documentation.
Any list types accepted by HexChat should be dynamically accepted by the
Python plugin interface.


##### channels

The channels list type gives you access to the channels, queries and
their servers. The folloing attributes are available in each list item:

 * **channel:** Channel or query name.
 * **chantypes:** Channel types e.g. #!&.
 * **context:** A context object, giving access to that channel/server.
 * **id:** Unique server id.
 * **lag:** Latency in milliseconds.
 * **maxmodes:** Max modes per line.
 * **network:** Network name to which this channel belongs.
 * **nickprefixes:** Nickname prefixes e.g. @%+.
 * **nickmodes:** Nickname mode chars e.g. ov.
 * **queue:** Number of bytes in the send-queue.  
 * **server:** Server name to which this channel belongs.
 * **users:** Number of users in the channel.
 * **type:** Type of context.
	* 1: Server
	* 2: Channel
	* 3: Dialog

 * **flags:** Bit field of flags:
    * 0: Connected
    * 1: Connecting
    * 2: Away
    * 3: End of MOTD (Login Complete)
    * 4: Has WHOX
    * 5: Has IDMSG
    * 6: Join/Parts hidden
    * 7: Unused
    * 8: Beep on Message
    * 9: Blink Tray
    * 10: Blink Task Bar


##### dcc

The dcc list type gives you access to a list of DCC file transfers. The
following attributes are available in each list item:

 * **address32:** Address of the remote user (ipv4 address, as an int).
 * **cps:** Bytes per second (speed).
 * **destfile:** Destination full pathname.
 * **file:** Filename.
 * **nick:** Nickname of person who the file is from/to.
 * **port:** TCP port number.
 * **pos:** Bytes sent/received.
 * **resume:** Point at which this file was resumed (or zero if it was not resumed).
 * **size:** File size in bytes.
 * **status:** DCC status:
	* 0: queued
	* 1: active
	* 2: failed
	* 3: done
	* 4: connecting
	* 5: aborted
 * **type:** DCC type:
	* 0: send
	* 1: receive
	* 2: chatrecv
	* 3: chatsend


##### users

The users list type gives you access to a list of users in the current
channel. The following attributes are available in each list item:

 * **away:** Away status.
 * **lasttalk:** Last time the user was seen talking.
 * **host:** Host name in the form user@host (or None, if not known).
 * **nick:** Nick name.
 * **prefix:** Prefix character, .e.g: @ or +. Points to a single char.
 * **realname:** Real name.
 * **selected:** Selected status in the userlist.


##### ignore

The ignore list type gives you access to the current ignored list. The
following attributes are available in each list item:

 * **mask:** Ignore mask (for example, "\*!\*@\*.aol.com").
 * **flags:** Bit field of flags:
	* 0: private
	* 1: notice
	* 2: channel
	* 3: ctcp
	* 4: invite
	* 5: unignore
	* 6: nosave
	* 7: dcc


### Hook functions

These functions allow one to hook into HexChat events.


#### Priorities

When a priority keyword parameter is accepted, it means that this
callback may be hooked with five different priorities: PRI\_HIGHEST,
PRI\_HIGH, PRI\_NORM, PRI\_LOW, and PRI\_LOWEST. The usage of these
constants, which are available in the xchat module, will define the
order in which your plugin will be called. Most of the time, you won't
want to change its default value (PRI\_NORM).


#### Parameters word and word_eol

These parameters, when available in a callback, are lists of strings
which contain the parameters the user entered for the particular
command. For example, if you executed:

<pre>
/command NICK Hi there!
</pre>

 * **word[0]** is `command`
 * **word[1]** is `NICK`
 * **word[2]** is `Hi`
 * **word[3]** is `there!`
 * **word\_eol[0]** is `command NICK Hi there!`
 * **word\_eol[1]** is `NICK Hi there!`
 * **word\_eol[2]** is `Hi there!`
 * **word\_eol[3]** is `there!`


#### Parameter userdata

The parameter userdata, if given, allows you to pass a custom object to
your callback.

#### Callback return constants (EAT\_*)

When a callback is supposed to return one of the EAT\_\* macros, it is
able control how HexChat will proceed after the callback returns. These
are the available constants, and their meanings:

 * **EAT_PLUGIN:** Don't let any other plugin receive this event.
 * **EAT_XCHAT:** Don't let HexChat treat this event as usual.
 * **EAT_ALL:** Eat the event completely.
 * **EAT_NONE:** Let everything happen as usual.

Returning `None` is the same as returning `EAT_NONE`.


#### xchat.hook\_command(name, callback, userdata=None, priority=PRI\_NORM, help=None)

This function allows you to hook into the name HexChat command. It means
that everytime you type `/name ...`, `callback` will be called.
Parameters `userdata` and `priority` have their meanings explained
above, and the parameter help, if given, allows you to pass a help text
which will be shown when `/help name` is executed. This function returns
a hook handler which may be used in the `xchat.unhook()` function. For
example:

<pre>
def onotice_cb(word, word_eol, userdata):
    if len(word) < 2:
        print "Second arg must be the message!"
    else:
        xchat.command("NOTICE @%s %s" % (xchat.get_info("channel"), word_eol[1]))
    return xchat.EAT_ALL

xchat.hook_command("ONOTICE", onotice_cb, help="/ONOTICE <message> Sends a notice to all ops")
</pre>

You may return one of `EAT_*` constants in the callback, to control
HexChat's behavior, as explained above.


#### xchat.hook\_print(name, callback, userdata=None, priority=PRI\_NORM)

This function allows you to register a callback to trap any print
events. The event names are available in the _Edit Event Texts_ window.
Parameters `userdata` and `priority` have their meanings explained
above. This function returns a hook handler which may be used in the
`xchat.unhook()` function. For example:

<pre>
def youpart_cb(word, word_eol, userdata):
    print "You have left channel", word[2]
    return xchat.EAT_XCHAT # Don't let HexChat do its normal printing

xchat.hook_print("You Part", youpart_cb)
</pre>

You may return one of `EAT_*` constants in the callback, to control
HexChat's behavior, as explained above.


#### xchat.hook\_server(name, callback, userdata=None, priority=PRI\_NORM)

This function allows you to register a callback to be called when a
certain server event occurs. You can use this to trap `PRIVMSG`,
`NOTICE`, `PART`, a server numeric, etc. Parameters `userdata` and
`priority` have their meanings explained above. This function returns a
hook handler which may be used in the `xchat.unhook()` function. For
example:

<pre>
def kick_cb(word, word_eol, userdata):
    print "%s was kicked from %s (%s)" % (word[3], word[2], word_eol[4])
    # Don't eat this event, let other plugins and HexChat see it too
    return xchat.EAT_NONE

xchat.hook_server("KICK", kick_cb)
</pre>

You may return one of `EAT_*` constants in the callback, to control
HexChat's behavior, as explained above.


#### xchat.hook\_timer(timeout, callback, userdata=None)

This function allows you to register a callback to be called every
timeout milliseconds. Parameters userdata and priority have their
meanings explained above. This function returns a hook handler which may
be used in the `xchat.unhook()` function. For example:

<pre>
myhook = None

def stop_cb(word, word_eol, userdata):
    global myhook
    if myhook is not None:
        xchat.unhook(myhook)
        myhook = None
        print "Timeout removed!"

def timeout_cb(userdata):
    print "Annoying message every 5 seconds! Type /STOP to stop it."
    return 1 # Keep the timeout going

myhook = xchat.hook_timer(5000, timeout_cb)
xchat.hook_command("STOP", stop_cb)
</pre>

If you return a true value from the callback, the timer will be keeped,
otherwise it is removed.


#### xchat.hook\_unload(timeout, callback, userdata=None)

This function allows you to register a callback to be called when the
plugin is going to be unloaded. Parameters `userdata` and `priority`
have their meanings explained above. This function returns a hook
handler which may be used in the `xchat.unhook()` function. For example:

<pre>
def unload_cb(userdata):
    print "We're being unloaded!"

xchat.hook_unload(unload_cb)
</pre>


#### xchat.unhook(handler)

Unhooks any hook registered with the hook functions above.


### Plugin preferences

You can use pluginpref to easily store and retrieve settings. This was added in the Python plugin version 0.9

#### xchat.set\_pluginpref(name, value)

If neccessary creates a .conf file in the HexChat config folder named addon\_python.conf and stores the value in it. Returns 1 on success, 0 on failure.

> Note: Until the plugin uses different a conf file per script it's recommened to use 'PluginName-SettingName' to avoid conflicts.


#### xchat.get\_pluginpref(name)

This will return the value of the variable of that name. If there is none by this name it will return `None`. Numbers are always returned as Integers.


#### xchat.del\_pluginpref(name)

Deletes the specified variable. Returns 1 on success (or never existing), 0 on failure.


#### xchat.list\_pluginpref()

Returns a list of all currently set preferences.


### Context handling

Below you will find information about how to work with contexts.


#### Context objects

As explained in the Context theory session above, contexts give access
to a specific channel/query/server tab of HexChat. Every function
available in the xchat module will be evaluated in the current context,
which will be specified by HexChat itself before passing control to the
module. Sometimes you may want to work in a specific context, and that's
where context objects come into play.

You may create a context object using the `xchat.get_context()` or
`xchat.find_context()`, functions as explained below, or trough the
`xchat.get_list()` function, as explained in its respective session.

Each context object offers the following methods:

 * **context.set():** Changes the current context to be the one represented by this context object.
 * **context.prnt(string):** Does the same as the xchat.prnt() function, but in the given context.
 * **context.emit\_print(event\_name, \*args):** Does the same as the emit\_print() function, but in the given context.
 * **context.command(string):** Does the same as the xchat.command() function, but in the given context.
 * **context.get\_info(type):** Does the same as the xchat.get\_info() function, but in the given context.
 * **context.get\_list(type):** Does the same as the xchat.get\_list() function, but in the given context.


#### xchat.get\_context()

Returns a context object corresponding the the current context.

#### xchat.find\_context(server=None, channel=None)

Finds a context based on a channel and servername. If `server` is
`None`, it finds any channel (or query) by the given name. If `channel`
is `None`, it finds the front-most tab/window of the given server. For
example:

<pre>
cnc = xchat.find_context(channel='#conectiva')
cnc.command('whois niemeyer')
</pre>

***

Original Author: Gustavo Niemeyer [gustavo@niemeyer.net](mailto:gustavo@niemeyer.net)

For purty html: `pandoc --toc python.md -s --highlight-style haddock -o python.html`
