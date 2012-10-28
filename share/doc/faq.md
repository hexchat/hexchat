# HexChat Frequently Asked Questions

## Using HexChat

### How do I autoconnect and join a channel when HexChat loads?

In the Network list select the Network you want to auto-connect to and
click Edit and turn ON the "Auto connect to this network at startup"
checkbox.

List channels in the favorites list to join them on connect.


### Why does HexChat join channels before identifying?

When using the nickserv password HexChat waits a short time before joining.
To change this value (which is in seconds) by running:
>   /set irc\_join\_delay number

The other option is SASL which is network dependent and can be enabled in the Network list (2.9.4+).


### How do I connect through a proxy?

Go to the menus, *Settings* -> *Preferences* -> *Network Setup* and fill in
the requested information there. Authentication (using a username and
password) is only supported for HTTP and Socks5.


### How do I show @ and + in front of nicknames that are Op and Voice when they talk?

To display @ and + characters next to nicknames as they talk, do the
following:

In the menus, open up Settings -> Text Events. Find the
*Channel Message* event in the list. The $3 code can be inserted to
print the user's mode-character (e.g. @ or +). For example, you might
want to change the default:

>   %C18%H<%H$4$1%H>%H%O$t$2

To

>   %C18%H<%H$4$3$1%H>%H%O$t$2 

Don't forget to **press Enter**, so the changes take effect in the list at
the top of the window.


### How do I change the Op and Voice userlist icons and Tree View icons?

You can override the default icons by placing PNG files with the names listed below in
the specified directory.

#### Files

**userlist:**

>   - op.png
>   - hop.png
>   - voice.png
>   - red.png *(1 level above op)*
>   - purple.png  *(2 levels above op)*

**channeltree:**

>   - server.png
>   - channel.png
>   - dialog.png
>   - util.png *(Channel List, DCC, etc (if enabled as tabs))*

**tray:**

>   - message.png
>   - highlight.png
>   - fileoffer.png
>   - hexchat.png

#### Locations

You will need to create the directory.

* Unix/Linux

>   ~/.config/hexchat/icons

* Windows

>   %APPDATA%\\HexChat\\icons

You can hide either of them in Preferences.


### How do I set different ban types?

1. Right click the nickname in the userlist, and choose a ban type from the "Kick/Ban" submenu.
2. You can also do it manually:
    >   /ban nick bantype where the bantype is a number from 0 to 3.
3. Or set the default with:

    >   /set irc\_ban\_type bantype sets the default ban type to use for all bans. The different types are:
    >
    >   -   0 = \*!*@*.host
    >   -   1 = \*!*@domain
    >   -   2 = \*!*user@*.host
    >   -   3 = \*!*user@domain

### Why does the timestamp overlap some nicknames?

Some networks allow very long nicknames (up to 32 letters). It can be
annoying to have the separator bar move too far to the right, just for
one long nick. Therefore, it has a set limit for the distance it will
move to the right. If you use a large font, you may need to adjust this
distance. It is set in pixels, for example:

>   /set text\_max\_indent 320

Once you adjust this setting high enough, overlapping timestamps and
nicknames should not occur. The adjustment will not take effect
immediately, a restart may be needed.

### How do I turn on Conference mode where I will not see join or part messages?

Right-click on the tab you want to change. In the submenu of the channel
name, there's a toggle-item "Show join/part messages", simply turn this
off.

If you want to turn this option on globally go to *Preferences* -> *Advanced*

Then all channels you join **after** setting this will start with "Show
join/part messages" turned off.


### Why doesn't DCC send work behind a router (IPNat/ADSL)?

If you are behind a IP-NAT or ADSL router, you will most likely have an
address like 192.168.0.1. This address is not usable on the Internet,
and must be translated.

When offering a DCC file, HexChat will tell the receiver your address.
If it says 192.168.0.1, the receiver will not be able to connect. One
way to make it send your "real" address is to enable the "Get my IP from
IRC Server" option in HexChat. This option is available in Preferences
-> File Transfers. When you turn it ON, you will have to re-login to
the server before it'll take effect.

You will also need to forward some ports for use in DCC send. You may
pick almost any port range you wish, for example, in HexChat set:

>   First DCC send port: 4990
>   Last DCC send port: 5000

This will allow you to send up to ten files at the same time, which
should be plenty for most people. Lastly, configure your router/modem to
forward ports 4990-5000 to your PC's address. You'll have to consult
your router/modem's manual on how to do this.


### How do I execute multiple commands in one line?

There are three ways to do this:

-   /LOAD -e <textfile>, where <textfile> is a full pathname to a
    file containing commands on each line.

-   Separate your commands with CTRL-SHIFT-u-a. This will appear as
     a little box with numbers onit.

-   You can create two UserCommands, with the same name, and then
    execute the UserCommand. It will be executed in the same order as
    it's written in the UserCommands GUI.


### I get this error: "Unknown file type abc.yz. Maybe you need to install the Perl or Python plugin?"

If you get this error when trying to load a Perl or Python script, it
means the plugin for running those scripts isn't loaded.

-   The Perl, Python and TCL plugins come with HexChat in the same
    archive.
-   During ./configure, it will check for Perl, Python and TCL libs and
    headers, you should check if it failed there.
-   The plugins directory can be found by issuing the shell command
    >   hexchat -p

-   All *.so files are auto-loaded at startup (*.dll on Windows).
-   If you downloaded a binary package, maybe the packager decided to
    exclude the Perl or Python plugins.


### How do I play sound files on certain events?

In the menus, go to: *Settings* -> *Preferences* -> *Sound*. Select the event
you want to make a sound on, then type in a sound filename (or use the
Browse button).


### How do I auto-load scripts at startup?

The root of your HexChat config is:

-   Windows: %APPDATA%\\HexChat
-   Unix/Linux: ~/.config/hexchat


Referred to as config from now. HexChat automatically loads, at startup:

>   - config/addons/*.lua Lua scripts
>   - config/addons/*.pl Perl scripts
>   - config/addons/*.py Python scripts
>   - config/addons/*.tcl Tcl scripts
>   - config/addons/*.dll Plugins (Windows)
>   - config/addons/*.so Plugins (Unix)

The addons dir may need to be created.

### How do I minimize HexChat to the System Tray (Notification Area)?

On both Unix and Windows there is an included tray plugin.
To enable minimizing to tray on exit:

>   /set gui\_tray\_flags -on 1

For minimizing to tray on minimize:

>   /set gui\_tray\_flags -on 4 *(use -off to disable)*

Alerts for this tray are in *Preferences* -> *Alerts*

The other option is Windows only, called HexTray. It is included with the installer.
Right click on the tray icon for its options.


### Where are the log files saved to?

* Unix/Linux

> ~/.config/hexchat/logs

* Windows

> %APPDATA%\\HexChat\\logs


### How do I rotate log files every so often?

By default settings, no rotation occurs, your log files will just keep getting larger.

Go to *Settings* -> *Preferences* -> *Logging* and change the log filename to any one of these:

>   %Y-%m-%d/%n-%c.log ->2006-12-30/FreeNode-\#channel.log
>
>   %n/%Y-%m-%d/%c.log ->FreeNode/2006-12-30/\#channel.log
>
>   %n/%c.log -> FreeNode/\#channel.log (no rotation)

%Y, %m and %d represents the current year, month and day respectively.
%n is the network name, e.g. "FreeNode" or "UnderNet", and finally, %c
is the channel. In these examples, a new log filename and folder would
be created after midnight.

 You can find more possibilities at
[http://xchat.org/docs/log/](http://xchat.org/docs/log/).

### Where did the Real Name field go?

The real name field is now removed from the Network List. This is in
order to avoid alienating newcomers (some might be afraid of their
personal data).

The network-specific real name can still be set via the GUI. If you want
to modify the global real name, just issue the following command:

>   /set irc\_real\_name Stewie Griffin


### How do I migrate my settings from XChat?

* UNIX/Linux

>   1. Copy ~/.xchat2 to ~/.config/hexchat
>   2. Rename ~/.config/hexchat/xchat.conf to ~/.config/hexchat/hexchat.conf
>   3. Rename ~/.config/hexchat/xchatlogs to ~/.config/hexchat/logs
>   4. Move all your 3rd party addons to ~/.config/hexchat/addons
>   5. Rename ~/.config/hexchat/plugin\_\*.conf to ~/.config/hexchat/addon\_\*.conf

* Windows

>   1. Copy *%APPDATA%\\X-Chat 2 to %APPDATA%\\HexChat
>   2. Rename *%APPDATA%\\HexChat\\xchat.conf to %APPDATA%\\HexChat\\hexchat.conf
>   3. Rename *%APPDATA%\\HexChat\\xchatlogs to %APPDATA%\\HexChat\\logs
>   4. Move all your 3rd party addons to %APPDATA%\\HexChat\\addons
>   5. Rename *%APPDATA%\\HexChat\\plugin\_\*.conf to %APPDATA%\\HexChat\\addon\_\*.conf


## Contributions, Development and Bugs.

### I found a bug, what can I do?

Firstly, make sure it's the latest stable version of HexChat.

If you still experience issues, you can search for the issue on
[Github](https://github.com/hexchat/hexchat/issues?state=open)
if it has not been reported open an issue with as much detail as possible.


### Can I write a new language translation for HexChat?

You sure can, but I don't accept translations directly. They must be
done through the [Transifex
Project](https://www.transifex.com/projects/p/hexchat/). You simply register
on the site, then you can apply for membership in a translation team via the web
interface. Approvals are done manually so it might take a few days for you to be
approved. Also bear in mind that the email address with which you register on
Transifex will be visible in the translation files.


* * * * *

For pretty html: `pandoc --toc -s faq.md -o faq.html`