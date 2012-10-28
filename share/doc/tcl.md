# HexChat Tcl Interface

(This file is currently not converted properly to Markdown format.)

<hr><b>Note to Eggdrop Scripters:</b>&nbsp; The Tcl Plugin for XChat will <b>not</b> run eggdrop scripts.
&nbsp;Contrary to popular belief, <b>Tcl was not invented by or for eggdrop.</b>  &nbsp;Eggdrop,
like many other successful projects is just another happy user of Tcl.  &nbsp;Tcl was around long
before Eggdrop and is broadly considered the industry standard language for automation.
<hr>

<a name='/reload'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>/reload - Clear and reload all tcl scripts.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>/reload</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Clears out and reloads all tcl scripts.  Any variables defined and any open files are lost.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#/source'>/source</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='/source'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>/source - Load a specific tcl script file.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>/source filename</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Loads a tcl script into XChat.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#/reload'>/reload</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='/tcl'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>/tcl - Execute any tcl command</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>/tcl command ?args?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Allows for the immediate execution of any tcl command.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>/tcl puts "Hello, XChat World!"
/tcl xchatdir</pre></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>

<p><h1>Tcl Plugin TCL Commands</h1>

<a name='alias'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>alias - Creates a new xchat command.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>alias name { script }</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Creates a new xchat command and executes <i>script</i> when that command is entered.
<br>
<br>
Upon executing the alias, the following variables will be set:
<br>

<br>
<table width=95% border=0 bgcolor=#dddddd cellpadding=1 cellspacing=2>
<tr>
<td> $_cmd     </td>
<td> the alias name
</td>
</tr>
<tr>
<td> $_rest    </td>
<td> params included with alias
</td>
</tr>
</table><br>

<br>
You can also hook all text (non-commands) sent to any given tab/window by pre-pending the name of any tab with an '@'.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre># do 'ls -al' command on any directory
alias ls {
  print "[eval "exec ls -al $_rest"]"
  complete
}

# uppercase everything I say in #somechannel
alias @#somechannel {
  /say [string toupper $_rest]
  complete
}

# brag about my uptime
alias uptime {
 /say [bold][me]'s Uptime:[bold] [string trim [exec uptime]]
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#complete'>complete</a>, <a href='#on'>on</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='away'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>away - Returns your /away message.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>away ?server|context?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns your /away message.  If no <b><i>server</i></b> or <b><i>context</i></b> is omitted, the current server is assumed.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>set awaymsg [away]</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='channel'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>channel - Return the current query/channel name.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>channel ?context?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the name of the current channel or query.  You may also specify a specific <b><i>context</i></b> to get the name of.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>set thischannel [channel]</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#channels'>channels</a>, <a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#server'>server</a>, <a href='#servers'>servers</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='channels'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>channels - Returns of list of all channels you are in.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>channels ?server|context?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns a list of all channels you are in. If <b><i>server</i></b> or <b><i>context</i></b> is omitted, the current server is assumed.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>alias mychannels {
  foreach s [servers] {
    print "Server: $s"
    foreach c [channels $s] {
      print " - Channel: $c - [topic $s $c]"
    }
  }
  complete
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#channel'>channel</a>, <a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#server'>server</a>, <a href='#servers'>servers</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='chats'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>chats - Returns a list of opened dcc chats.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>chats</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the name of the current active dcc chats.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>set mychats [chats]
print "I am directly connected to [join $mychats ", "]"</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#channels'>channels</a>, <a href='#dcclist'>dcclist</a>, <a href='#queries'>queries</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='command'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>command - Simulate a command entered into xchat.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>command ?server|context? ?channel|nick? text</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Executes any internal or external chat command as if it had been typed into xchat directly.  If <b><i>server</i></b> or <b><i>channel|nick</i></b> are omitted, the current ones are assumed.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>command "whois [me]"
command #mychannel "me wonders what this does."
command irc.myserver.com #thatchannel "say Hello, World!"
command irc.nyserver.com "away I'm gone"</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#raw'>raw</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='complete'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>complete - Set return mode of an 'on' or 'alias' script</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>complete ?retcode?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Similar to TCL's <i>return</i> command, complete halts further processing of an <i>on</i> or <i>alias</i> script and sets a return value.
<br>

<br>
<table width=95% border=0 bgcolor=#dddddd cellpadding=1 cellspacing=2>
<tr>
<td> EAT_NONE    </td>
<td> Allows all other plugins and xchat to see this event.
</td>
</tr>
<tr>
<td> EAT_XCHAT   </td>
<td> Halts further processing by xchat
</td>
</tr>
<tr>
<td> EAT_PLUGIN  </td>
<td> Halts further processing by other plugins (default).
</td>
</tr>
<tr>
<td> EAT_ALL     </td>
<td> Halts further processing by other plugins and xchat.</td>
</tr>
</table><br>
</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>on XC_TABOPEN whatever {
  print "Hello from [channel]"
  complete
}

alias bar {
  /me has been on irc long enough to still be traumatized by !bar scripts.
  complete
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#alias'>alias</a>, <a href='#on'>on</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='dcclist'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>dcclist - Returns detailed information about all dcc chats and files transfers.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>dcclist</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns a list of all dcc chats and transfers.
<br>

<br>
Each list entry is made up of the following elements:
<br>

<br>
<table width=95% border=0 bgcolor=#dddddd cellpadding=1 cellspacing=2>
<tr>
<td>type </td>
<td> chatsend, chatrecv, filesend, filerecv.
</td>
</tr>
<tr>
<td>status </td>
<td> queued, active, failed, done, connecting, aborted.
</td>
</tr>
<tr>
<td>nick </td>
<td> Nick of other user.
</td>
</tr>
<tr>
<td>filename </td>
<td> Name of file being sent or reveived.
</td>
</tr>
<tr>
<td>size </td>
<td> size of file being sent or reveived.
</td>
</tr>
<tr>
<td>resume </td>
<td> resume position of file being sent or reveived.
</td>
</tr>
<tr>
<td>pos </td>
<td> current position of file being sent or reveived.
</td>
</tr>
<tr>
<td>cps </td>
<td> current transfer speed in bytes per second.
</td>
</tr>
<tr>
<td>address </td>
<td> address of remote connection.
</td>
</tr>
<tr>
<td>port </td>
<td> port of the remote connection.</td>
</tr>
</table><br>
</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>foreach entry [dcclist] {
  print "$entry"
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#chats'>chats</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='findcontext'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>findcontext - Finds a context based on a channel and/or server name.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>findcontext ?server? ?channel|nick?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Finds a context based on a channel and/or server name. If the <b><i>server</i></b> is omitted, it finds any channel (or query) by the given name on the current server. If <b><i>channel|nick</i></b> is omitted, it finds the default server tab for that server.
</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>set context [findcontext irc.whatever.com]
set context [findcontext #mychannel]
set context [findcontext irc.whatever.com #thatchannel]
set context [findcontext]</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>Notes:</b></td>
<td>This function is not normally needed with the tclplugin.  It is included only to add completeness with the XChat C API.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#getcontext'>getcontext</a>, <a href='#setcontext'>setcontext</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='getcontext'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>getcontext - Returns the current context for your plugin.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>getcontext</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the current context for your plugin. You can use this later with <b><i>setcontext.</i></b></td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>set context [getcontext]</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>Notes:</b></td>
<td>This function is not normally needed with the tclplugin.  It is included only to add completeness with the XChat C API.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#setcontext'>setcontext</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='getinfo'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>getinfo - Returns information based on your current context.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>getinfo field</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Provides direct access to XChat C API command xhat_get_info.  Most of these have replacement tcl plugin commands that offer more functionality.
<br>

<br>
The following fields are currently defined:
<br>

<br>
<table width=95% border=0 bgcolor=#dddddd cellpadding=1 cellspacing=2>
<tr>
<td>away </td>
<td> away reason or NULL if you are not away.
</td>
</tr>
<tr>
<td>channel </td>
<td> current channel name.
</td>
</tr>
<tr>
<td>host </td>
<td> real hostname of the server you connected to.
</td>
</tr>
<tr>
<td>network </td>
<td> current network name or NULL.
</td>
</tr>
<tr>
<td>nick </td>
<td> your current nick name.
</td>
</tr>
<tr>
<td>server </td>
<td> current server name (what the server claims to be).
</td>
</tr>
<tr>
<td>topic </td>
<td> current channel topic.
</td>
</tr>
<tr>
<td>version </td>
<td> xchat version number.
</td>
</tr>
<tr>
<td>xchatdir </td>
<td> xchat config directory, e.g.: /home/user/.xchat.</td>
</tr>
</table><br>
</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>print "I am using XChat [getinfo version]"</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#away'>away</a>, <a href='#channel'>channel</a>, <a href='#host'>host</a>, <a href='#me'>me</a>, <a href='#network'>network</a>, <a href='#server'>server</a>, <a href='#topic'>topic</a>, <a href='#version'>version</a>, <a href='#xchatdir'>xchatdir</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='getlist'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>getlist - Returns information from XChats list of lists</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>getlist ?listname?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns a list of information from XChat's internal list of lists.  If <b><i>listname</i></b> is omitted, the names of all the available lists are returned.
<br>

<br>
The first entry in the list is the names of all the fields for that list.  The rest of list are the actual list entries.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#channels'>channels</a>, <a href='#dcclist'>dcclist</a>, <a href='#ignores'>ignores</a>, <a href='#queries'>queries</a>, <a href='#servers'>servers</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='host'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>host - Returns the hostname of the server.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>host ?server|context?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the hostname of the server you connected to.  If you connected to a networks round-robin name, e.g. irc.openprojects.org, irc.newnet.net, etc., it will return that name.  If <b><i>server</i></b> is omitted, the current one is assumed.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>print "I attempted to connect to [host] on [network]."
print "I am actually connected to [server]."</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>Notes:</b></td>
<td>If you want to know the exact server name, use <b><i>server</i></b>.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#network'>network</a>, <a href='#server'>server</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='ignores'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>ignores - Returns list of ignored hosts.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>ignores</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns a list of all ignored hosts.
<br>

<br>
Each list entry is made up the hostmask being ignored, followed by a sub-list of the types of ignores on that mask.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>set ignorelist [ignores]
foreach entry $ignorelist {
  print "Ignoring:"
  print "[lindex $entry 0]: [lindex $entry 1]"
}</pre></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='killtimer'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>killtimer - Kills the specified timer.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>killtimer timerID</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Removes the specified timerID from the timer queue.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#timer'>timer</a>, <a href='#timerexists'>timerexists</a>, <a href='#timers'>timers</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='me'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>me - Returns your nick.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>me ?server|context?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns your current nick. If <b><i>server</i></b> is omitted, the current one is used by default.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='network'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>network - Returns the name of the network.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>network ?server|context?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the name of the network, relative to the server list, that you are connected to.  If no <b><i>server</i></b>is omitted, the current one current one is used by default.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>print "I attempted to connect to [host] on [network]."
print "I am actually connected to [server]."</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#host'>host</a>, <a href='#server'>server</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='nickcmp'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>nickcmp - Performs an RFC1459 compliant string compare.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>nickcmp string1 string2</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>This command performs an RFC1459 compliant string compare.  Use this to compare channels and nicknames. The function works the same way as strcasecmp.
<br>

<br>
Because of IRC's scandanavian origin, the characters {}| are considered to be the lower case equivalents of the characters [], respectively. This is a critical issue when determining the equivalence of two nicknames.</td>
</tr>

<tr valign=top>
<td align=right><b>Returns:</b></td>
<td>An integer less than, equal to, or greater than zero if string1 is found, respectively, to be less than, to match, or be greater than string2.</td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='off'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>off - Removes a script previously assigned with <b><i>on</i></b></td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>off token ?label?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Removes a script from the specified XChat <b><i>token</i></b> and <b><i>label</i></b>.  If <b><i>label</i></b> is omitted, all scripts for that token are removed.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#on'>on</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='on'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>on - Execute a command on an irc event</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>on token label { script | procname }</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Whenever <b><i>token</i></b> is triggered, <b><i>script</i></b> will be executed. <b><i>label</i></b> is some descriptive word that identifies which script is being executed when you have multiple scripts assigned to the same event.  It is suggested that you use your initials or the name of your script as the 'label'.
<br>

<br>
The <b><i>token</i></b> can be any server token or an internal XChat event.  When executing your script, the following variables will be set:
<br>

<br>
<table width=95% border=0 bgcolor=#dddddd cellpadding=1 cellspacing=2>
<tr>
<td> $_label   </td>
<td> As defined by the 'on' command.
</td>
</tr>
<tr>
<td> $_src     </td>
<td> source of the event.  nick!ident@host -or- irc.servername.com
</td>
</tr>
<tr>
<td> $_cmd     </td>
<td> irc command.  JOIN, PRIVMSG, KICK, etc.
</td>
</tr>
<tr>
<td> $_dest    </td>
<td> intended target of this event.  nick, </td>
</tr>
<tr>
<td> $_rest    </td>
<td> the rest of the message.
</td>
</tr>
<tr>
<td> $_raw     </td>
<td> the raw line received from the irc server.
</td>
</tr>
<tr>
<td> $_private </td>
<td> '0' means the message was public, '1' = private.
</td>
</tr>
</table><br>

<br>
You may further use <b><i>splitsrc</i></b> command to create the additional variables:
<br>

<br>
<table width=95% border=0 bgcolor=#dddddd cellpadding=1 cellspacing=2>
<tr>
<td> $_nick    </td>
<td> irc user nick extracted from $_src
</td>
</tr>
<tr>
<td> $_ident   </td>
<td> irc user ident extracted from $_src
</td>
</tr>
<tr>
<td> $_host    </td>
<td> irc user hostname extracted from $_src
</td>
</tr>
</table><br>

<br>
For channel management scripts, you may use any word with '!' in front (e.g. !pingme") as the token.  Any time someone uses that command in a channel or in a private message, the script will be executed.
<br>

<br>
The following custom XChat internal token are also available:
<br>

<br>
<table width=95% border=0 bgcolor=#dddddd cellpadding=1 cellspacing=2>
<tr>
<td> INVITE </td>
<td> (rfc1459) Invited to channel.
</td>
</tr>
<tr>
<td> JOIN </td>
<td> (rfc1459) Joined a channel
</td>
</tr>
<tr>
<td> KICK </td>
<td> (rfc1459) Kicked from a channel
</td>
</tr>
<tr>
<td> KILL </td>
<td> (rfc1459) Killed from server
</td>
</tr>
<tr>
<td> MODE </td>
<td> (rfc1459) Channel or User mode change
</td>
</tr>
<tr>
<td> NICK </td>
<td> (rfc1459) Nick change.
</td>
</tr>
<tr>
<td> NOTICE </td>
<td> (rfc1459) Private Notice
</td>
</tr>
<tr>
<td> PART </td>
<td> (rfc1459) Parted a channel
</td>
</tr>
<tr>
<td> PING </td>
<td> (rfc1459) Server Ping
</td>
</tr>
<tr>
<td> PRIVMSG </td>
<td> (rfc1459) Private Message
</td>
</tr>
<tr>
<td> QUIT </td>
<td> (rfc1459) Quit the server.
</td>
</tr>
<tr>
<td> TOPIC </td>
<td> (rfc1459) Channel topic change
</td>
</tr>
<tr>
<td> WALLOPS </td>
<td> (rfc1459) Wallops
</td>
</tr>
<tr>
<td> ACTION </td>
<td> Incoming /me whatever action command.
</td>
</tr>
<tr>
<td> CHAT </td>
<td> Incoming line of text from dcc chat conversation.
</td>
</tr>
<tr>
<td> CTCP </td>
<td> Incoming CTCP (PING, VERSION, etc)
</td>
</tr>
<tr>
<td> CTCR </td>
<td> Incoming reply from your CTCP to someone else.
</td>
</tr>
<tr>
<td> SNOTICE </td>
<td> Incoming notice from a server.
</td>
</tr>
<tr>
<td> 001 </td>
<td> (rfc1459) RPL_WELCOME
</td>
</tr>
<tr>
<td> 002 </td>
<td> (rfc1459) RPL_YOURHOST
</td>
</tr>
<tr>
<td> 003 </td>
<td> (rfc1459) RPL_CREATED
</td>
</tr>
<tr>
<td> 004 </td>
<td> (rfc1459) RPL_MYINFO
</td>
</tr>
<tr>
<td> 005 </td>
<td> (rfc1459) RPL_PROTOCTL
</td>
</tr>
<tr>
<td> 006 </td>
<td> (rfc1459) RPL_MAP
</td>
</tr>
<tr>
<td> 007 </td>
<td> (rfc1459) RPL_MAPEND
</td>
</tr>
<tr>
<td> 200 </td>
<td> (rfc1459) RPL_TRACELINK
</td>
</tr>
<tr>
<td> 201 </td>
<td> (rfc1459) RPL_TRACECONNECTING
</td>
</tr>
<tr>
<td> 202 </td>
<td> (rfc1459) RPL_TRACEHANDSHAKE
</td>
</tr>
<tr>
<td> 203 </td>
<td> (rfc1459) RPL_TRACEUNKNOWN
</td>
</tr>
<tr>
<td> 204 </td>
<td> (rfc1459) RPL_TRACEOPERATOR
</td>
</tr>
<tr>
<td> 205 </td>
<td> (rfc1459) RPL_TRACEUSER
</td>
</tr>
<tr>
<td> 206 </td>
<td> (rfc1459) RPL_TRACESERVER
</td>
</tr>
<tr>
<td> 207 </td>
<td> (rfc1459) RPL_TRACESERVICE
</td>
</tr>
<tr>
<td> 208 </td>
<td> (rfc1459) RPL_TRACENEWTYPE
</td>
</tr>
<tr>
<td> 209 </td>
<td> (rfc1459) RPL_TRACECLASS
</td>
</tr>
<tr>
<td> 211 </td>
<td> (rfc1459) RPL_STATSLINKINFO
</td>
</tr>
<tr>
<td> 212 </td>
<td> (rfc1459) RPL_STATSCOMMANDS
</td>
</tr>
<tr>
<td> 213 </td>
<td> (rfc1459) RPL_STATSCLINE
</td>
</tr>
<tr>
<td> 214 </td>
<td> (rfc1459) RPL_STATSOLDNLINE
</td>
</tr>
<tr>
<td> 215 </td>
<td> (rfc1459) RPL_STATSILINE
</td>
</tr>
<tr>
<td> 216 </td>
<td> (rfc1459) RPL_STATSKLINE
</td>
</tr>
<tr>
<td> 217 </td>
<td> (rfc1459) RPL_STATSQLINE
</td>
</tr>
<tr>
<td> 218 </td>
<td> (rfc1459) RPL_STATSYLINE
</td>
</tr>
<tr>
<td> 219 </td>
<td> (rfc1459) RPL_ENDOFSTATS
</td>
</tr>
<tr>
<td> 220 </td>
<td> (rfc1459) RPL_STATSBLINE
</td>
</tr>
<tr>
<td> 221 </td>
<td> (rfc1459) RPL_UMODEIS
</td>
</tr>
<tr>
<td> 222 </td>
<td> (rfc1459) RPL_SQLINE_NICK
</td>
</tr>
<tr>
<td> 223 </td>
<td> (rfc1459) RPL_STATSGLINE
</td>
</tr>
<tr>
<td> 224 </td>
<td> (rfc1459) RPL_STATSTLINE
</td>
</tr>
<tr>
<td> 225 </td>
<td> (rfc1459) RPL_STATSELINE
</td>
</tr>
<tr>
<td> 226 </td>
<td> (rfc1459) RPL_STATSNLINE
</td>
</tr>
<tr>
<td> 227 </td>
<td> (rfc1459) RPL_STATSVLINE
</td>
</tr>
<tr>
<td> 231 </td>
<td> (rfc1459) RPL_SERVICEINFO
</td>
</tr>
<tr>
<td> 232 </td>
<td> (rfc1459) RPL_RULES
</td>
</tr>
<tr>
<td> 233 </td>
<td> (rfc1459) RPL_SERVICE
</td>
</tr>
<tr>
<td> 234 </td>
<td> (rfc1459) RPL_SERVLIST
</td>
</tr>
<tr>
<td> 235 </td>
<td> (rfc1459) RPL_SERVLISTEND
</td>
</tr>
<tr>
<td> 241 </td>
<td> (rfc1459) RPL_STATSLLINE
</td>
</tr>
<tr>
<td> 242 </td>
<td> (rfc1459) RPL_STATSUPTIME
</td>
</tr>
<tr>
<td> 243 </td>
<td> (rfc1459) RPL_STATSOLINE
</td>
</tr>
<tr>
<td> 244 </td>
<td> (rfc1459) RPL_STATSHLINE
</td>
</tr>
<tr>
<td> 245 </td>
<td> (rfc1459) RPL_STATSSLINE
</td>
</tr>
<tr>
<td> 247 </td>
<td> (rfc1459) RPL_STATSXLINE
</td>
</tr>
<tr>
<td> 248 </td>
<td> (rfc1459) RPL_STATSULINE
</td>
</tr>
<tr>
<td> 249 </td>
<td> (rfc1459) RPL_STATSDEBUG
</td>
</tr>
<tr>
<td> 250 </td>
<td> (rfc1459) RPL_STATSCONN
</td>
</tr>
<tr>
<td> 251 </td>
<td> (rfc1459) RPL_LUSERCLIENT
</td>
</tr>
<tr>
<td> 252 </td>
<td> (rfc1459) RPL_LUSEROP
</td>
</tr>
<tr>
<td> 253 </td>
<td> (rfc1459) RPL_LUSERUNKNOWN
</td>
</tr>
<tr>
<td> 254 </td>
<td> (rfc1459) RPL_LUSERCHANNELS
</td>
</tr>
<tr>
<td> 255 </td>
<td> (rfc1459) RPL_LUSERME
</td>
</tr>
<tr>
<td> 256 </td>
<td> (rfc1459) RPL_ADMINME
</td>
</tr>
<tr>
<td> 257 </td>
<td> (rfc1459) RPL_ADMINLOC1
</td>
</tr>
<tr>
<td> 258 </td>
<td> (rfc1459) RPL_ADMINLOC2
</td>
</tr>
<tr>
<td> 259 </td>
<td> (rfc1459) RPL_ADMINEMAIL
</td>
</tr>
<tr>
<td> 261 </td>
<td> (rfc1459) RPL_TRACELOG
</td>
</tr>
<tr>
<td> 265 </td>
<td> (rfc1459) RPL_LOCALUSERS
</td>
</tr>
<tr>
<td> 266 </td>
<td> (rfc1459) RPL_GLOBALUSERS
</td>
</tr>
<tr>
<td> 271 </td>
<td> (rfc1459) RPL_SILELIST
</td>
</tr>
<tr>
<td> 272 </td>
<td> (rfc1459) RPL_ENDOFSILELIST
</td>
</tr>
<tr>
<td> 275 </td>
<td> (rfc1459) RPL_STATSDLINE
</td>
</tr>
<tr>
<td> 290 </td>
<td> (rfc1459) RPL_HELPHDR
</td>
</tr>
<tr>
<td> 291 </td>
<td> (rfc1459) RPL_HELPOP
</td>
</tr>
<tr>
<td> 292 </td>
<td> (rfc1459) RPL_HELPTLR
</td>
</tr>
<tr>
<td> 293 </td>
<td> (rfc1459) RPL_HELPHLP
</td>
</tr>
<tr>
<td> 294 </td>
<td> (rfc1459) RPL_HELPFWD
</td>
</tr>
<tr>
<td> 295 </td>
<td> (rfc1459) RPL_HELPIGN
</td>
</tr>
<tr>
<td> 300 </td>
<td> (rfc1459) RPL_NONE
</td>
</tr>
<tr>
<td> 301 </td>
<td> (rfc1459) RPL_AWAY
</td>
</tr>
<tr>
<td> 302 </td>
<td> (rfc1459) RPL_USERHOST
</td>
</tr>
<tr>
<td> 303 </td>
<td> (rfc1459) RPL_ISON
</td>
</tr>
<tr>
<td> 304 </td>
<td> (rfc1459) RPL_TEXT
</td>
</tr>
<tr>
<td> 305 </td>
<td> (rfc1459) RPL_UNAWAY
</td>
</tr>
<tr>
<td> 306 </td>
<td> (rfc1459) RPL_NOWAWAY
</td>
</tr>
<tr>
<td> 307 </td>
<td> (rfc1459) RPL_WHOISREGNICK
</td>
</tr>
<tr>
<td> 308 </td>
<td> (rfc1459) RPL_RULESSTART
</td>
</tr>
<tr>
<td> 309 </td>
<td> (rfc1459) RPL_ENDOFRULES
</td>
</tr>
<tr>
<td> 310 </td>
<td> (rfc1459) RPL_WHOISHELPOP
</td>
</tr>
<tr>
<td> 311 </td>
<td> (rfc1459) RPL_WHOISUSER
</td>
</tr>
<tr>
<td> 312 </td>
<td> (rfc1459) RPL_WHOISSERVER
</td>
</tr>
<tr>
<td> 313 </td>
<td> (rfc1459) RPL_WHOISOPERATOR
</td>
</tr>
<tr>
<td> 314 </td>
<td> (rfc1459) RPL_WHOWASUSER
</td>
</tr>
<tr>
<td> 315 </td>
<td> (rfc1459) RPL_ENDOFWHO
</td>
</tr>
<tr>
<td> 316 </td>
<td> (rfc1459) RPL_WHOISCHANOP
</td>
</tr>
<tr>
<td> 317 </td>
<td> (rfc1459) RPL_WHOISIDLE
</td>
</tr>
<tr>
<td> 318 </td>
<td> (rfc1459) RPL_ENDOFWHOIS
</td>
</tr>
<tr>
<td> 319 </td>
<td> (rfc1459) RPL_WHOISCHANNELS
</td>
</tr>
<tr>
<td> 320 </td>
<td> (rfc1459) RPL_WHOISSPECIAL
</td>
</tr>
<tr>
<td> 321 </td>
<td> (rfc1459) RPL_LISTSTART
</td>
</tr>
<tr>
<td> 322 </td>
<td> (rfc1459) RPL_LIST
</td>
</tr>
<tr>
<td> 323 </td>
<td> (rfc1459) RPL_LISTEND
</td>
</tr>
<tr>
<td> 324 </td>
<td> (rfc1459) RPL_CHANNELMODEIS
</td>
</tr>
<tr>
<td> 329 </td>
<td> (rfc1459) RPL_CREATIONTIME
</td>
</tr>
<tr>
<td> 331 </td>
<td> (rfc1459) RPL_NOTOPIC
</td>
</tr>
<tr>
<td> 332 </td>
<td> (rfc1459) RPL_TOPIC
</td>
</tr>
<tr>
<td> 333 </td>
<td> (rfc1459) RPL_TOPICWHOTIME
</td>
</tr>
<tr>
<td> 334 </td>
<td> (rfc1459) RPL_LISTSYNTAX
</td>
</tr>
<tr>
<td> 335 </td>
<td> (rfc1459) RPL_WHOISBOT
</td>
</tr>
<tr>
<td> 341 </td>
<td> (rfc1459) RPL_INVITING
</td>
</tr>
<tr>
<td> 342 </td>
<td> (rfc1459) RPL_SUMMONING
</td>
</tr>
<tr>
<td> 343 </td>
<td> (rfc1459) RPL_TICKER
</td>
</tr>
<tr>
<td> 346 </td>
<td> (rfc1459) RPL_INVITELIST
</td>
</tr>
<tr>
<td> 347 </td>
<td> (rfc1459) RPL_ENDOFINVITELIST
</td>
</tr>
<tr>
<td> 348 </td>
<td> (rfc1459) RPL_EXLIST
</td>
</tr>
<tr>
<td> 349 </td>
<td> (rfc1459) RPL_ENDOFEXLIST
</td>
</tr>
<tr>
<td> 351 </td>
<td> (rfc1459) RPL_VERSION
</td>
</tr>
<tr>
<td> 352 </td>
<td> (rfc1459) RPL_WHOREPLY
</td>
</tr>
<tr>
<td> 353 </td>
<td> (rfc1459) RPL_NAMREPLY
</td>
</tr>
<tr>
<td> 361 </td>
<td> (rfc1459) RPL_KILLDONE
</td>
</tr>
<tr>
<td> 362 </td>
<td> (rfc1459) RPL_CLOSING
</td>
</tr>
<tr>
<td> 363 </td>
<td> (rfc1459) RPL_CLOSEEND
</td>
</tr>
<tr>
<td> 364 </td>
<td> (rfc1459) RPL_LINKS
</td>
</tr>
<tr>
<td> 365 </td>
<td> (rfc1459) RPL_ENDOFLINKS
</td>
</tr>
<tr>
<td> 366 </td>
<td> (rfc1459) RPL_ENDOFNAMES
</td>
</tr>
<tr>
<td> 367 </td>
<td> (rfc1459) RPL_BANLIST
</td>
</tr>
<tr>
<td> 368 </td>
<td> (rfc1459) RPL_ENDOFBANLIST
</td>
</tr>
<tr>
<td> 369 </td>
<td> (rfc1459) RPL_ENDOFWHOWAS
</td>
</tr>
<tr>
<td> 371 </td>
<td> (rfc1459) RPL_INFO
</td>
</tr>
<tr>
<td> 372 </td>
<td> (rfc1459) RPL_MOTD
</td>
</tr>
<tr>
<td> 373 </td>
<td> (rfc1459) RPL_INFOSTART
</td>
</tr>
<tr>
<td> 374 </td>
<td> (rfc1459) RPL_ENDOFINFO
</td>
</tr>
<tr>
<td> 375 </td>
<td> (rfc1459) RPL_MOTDSTART
</td>
</tr>
<tr>
<td> 376 </td>
<td> (rfc1459) RPL_ENDOFMOTD
</td>
</tr>
<tr>
<td> 378 </td>
<td> (rfc1459) RPL_WHOISHOST
</td>
</tr>
<tr>
<td> 379 </td>
<td> (rfc1459) RPL_WHOISMODES
</td>
</tr>
<tr>
<td> 381 </td>
<td> (rfc1459) RPL_YOUREOPER
</td>
</tr>
<tr>
<td> 382 </td>
<td> (rfc1459) RPL_REHASHING
</td>
</tr>
<tr>
<td> 383 </td>
<td> (rfc1459) RPL_YOURESERVICE
</td>
</tr>
<tr>
<td> 384 </td>
<td> (rfc1459) RPL_MYPORTIS
</td>
</tr>
<tr>
<td> 385 </td>
<td> (rfc1459) RPL_NOTOPERANYMORE
</td>
</tr>
<tr>
<td> 386 </td>
<td> (rfc1459) RPL_QLIST
</td>
</tr>
<tr>
<td> 387 </td>
<td> (rfc1459) RPL_ENDOFQLIST
</td>
</tr>
<tr>
<td> 388 </td>
<td> (rfc1459) RPL_ALIST
</td>
</tr>
<tr>
<td> 389 </td>
<td> (rfc1459) RPL_ENDOFALIST
</td>
</tr>
<tr>
<td> 391 </td>
<td> (rfc1459) RPL_TIME
</td>
</tr>
<tr>
<td> 392 </td>
<td> (rfc1459) RPL_USERSSTART
</td>
</tr>
<tr>
<td> 393 </td>
<td> (rfc1459) RPL_USERS
</td>
</tr>
<tr>
<td> 394 </td>
<td> (rfc1459) RPL_ENDOFUSERS
</td>
</tr>
<tr>
<td> 395 </td>
<td> (rfc1459) RPL_NOUSERS
</td>
</tr>
<tr>
<td> 401 </td>
<td> (rfc1459) ERR_NOSUCHNICK
</td>
</tr>
<tr>
<td> 402 </td>
<td> (rfc1459) ERR_NOSUCHSERVER
</td>
</tr>
<tr>
<td> 403 </td>
<td> (rfc1459) ERR_NOSUCHCHANNEL
</td>
</tr>
<tr>
<td> 404 </td>
<td> (rfc1459) ERR_CANNOTSENDTOCHAN
</td>
</tr>
<tr>
<td> 405 </td>
<td> (rfc1459) ERR_TOOMANYCHANNELS
</td>
</tr>
<tr>
<td> 406 </td>
<td> (rfc1459) ERR_WASNOSUCHNICK
</td>
</tr>
<tr>
<td> 407 </td>
<td> (rfc1459) ERR_TOOMANYTARGETS
</td>
</tr>
<tr>
<td> 408 </td>
<td> (rfc1459) ERR_NOSUCHSERVICE
</td>
</tr>
<tr>
<td> 409 </td>
<td> (rfc1459) ERR_NOORIGIN
</td>
</tr>
<tr>
<td> 411 </td>
<td> (rfc1459) ERR_NORECIPIENT
</td>
</tr>
<tr>
<td> 412 </td>
<td> (rfc1459) ERR_NOTEXTTOSEND
</td>
</tr>
<tr>
<td> 413 </td>
<td> (rfc1459) ERR_NOTOPLEVEL
</td>
</tr>
<tr>
<td> 414 </td>
<td> (rfc1459) ERR_WILDTOPLEVEL
</td>
</tr>
<tr>
<td> 421 </td>
<td> (rfc1459) ERR_UNKNOWNCOMMAND
</td>
</tr>
<tr>
<td> 422 </td>
<td> (rfc1459) ERR_NOMOTD
</td>
</tr>
<tr>
<td> 423 </td>
<td> (rfc1459) ERR_NOADMININFO
</td>
</tr>
<tr>
<td> 424 </td>
<td> (rfc1459) ERR_FILEERROR
</td>
</tr>
<tr>
<td> 425 </td>
<td> (rfc1459) ERR_NOOPERMOTD
</td>
</tr>
<tr>
<td> 431 </td>
<td> (rfc1459) ERR_NONICKNAMEGIVEN
</td>
</tr>
<tr>
<td> 432 </td>
<td> (rfc1459) ERR_ERRONEUSNICKNAME
</td>
</tr>
<tr>
<td> 433 </td>
<td> (rfc1459) ERR_NICKNAMEINUSE
</td>
</tr>
<tr>
<td> 434 </td>
<td> (rfc1459) ERR_NORULES
</td>
</tr>
<tr>
<td> 435 </td>
<td> (rfc1459) ERR_SERVICECONFUSED
</td>
</tr>
<tr>
<td> 436 </td>
<td> (rfc1459) ERR_NICKCOLLISION
</td>
</tr>
<tr>
<td> 437 </td>
<td> (rfc1459) ERR_BANNICKCHANGE
</td>
</tr>
<tr>
<td> 438 </td>
<td> (rfc1459) ERR_NCHANGETOOFAST
</td>
</tr>
<tr>
<td> 439 </td>
<td> (rfc1459) ERR_TARGETTOOFAST
</td>
</tr>
<tr>
<td> 440 </td>
<td> (rfc1459) ERR_SERVICESDOWN
</td>
</tr>
<tr>
<td> 441 </td>
<td> (rfc1459) ERR_USERNOTINCHANNEL
</td>
</tr>
<tr>
<td> 442 </td>
<td> (rfc1459) ERR_NOTONCHANNEL
</td>
</tr>
<tr>
<td> 443 </td>
<td> (rfc1459) ERR_USERONCHANNEL
</td>
</tr>
<tr>
<td> 444 </td>
<td> (rfc1459) ERR_NOLOGIN
</td>
</tr>
<tr>
<td> 445 </td>
<td> (rfc1459) ERR_SUMMONDISABLED
</td>
</tr>
<tr>
<td> 446 </td>
<td> (rfc1459) ERR_USERSDISABLED
</td>
</tr>
<tr>
<td> 447 </td>
<td> (rfc1459) ERR_NONICKCHANGE
</td>
</tr>
<tr>
<td> 451 </td>
<td> (rfc1459) ERR_NOTREGISTERED
</td>
</tr>
<tr>
<td> 455 </td>
<td> (rfc1459) ERR_HOSTILENAME
</td>
</tr>
<tr>
<td> 459 </td>
<td> (rfc1459) ERR_NOHIDING
</td>
</tr>
<tr>
<td> 460 </td>
<td> (rfc1459) ERR_NOTFORHALFOPS
</td>
</tr>
<tr>
<td> 461 </td>
<td> (rfc1459) ERR_NEEDMOREPARAMS
</td>
</tr>
<tr>
<td> 462 </td>
<td> (rfc1459) ERR_ALREADYREGISTRED
</td>
</tr>
<tr>
<td> 463 </td>
<td> (rfc1459) ERR_NOPERMFORHOST
</td>
</tr>
<tr>
<td> 464 </td>
<td> (rfc1459) ERR_PASSWDMISMATCH
</td>
</tr>
<tr>
<td> 465 </td>
<td> (rfc1459) ERR_YOUREBANNEDCREEP
</td>
</tr>
<tr>
<td> 466 </td>
<td> (rfc1459) ERR_YOUWILLBEBANNED
</td>
</tr>
<tr>
<td> 467 </td>
<td> (rfc1459) ERR_KEYSET
</td>
</tr>
<tr>
<td> 468 </td>
<td> (rfc1459) ERR_ONLYSERVERSCANCHANGE
</td>
</tr>
<tr>
<td> 469 </td>
<td> (rfc1459) ERR_LINKSET
</td>
</tr>
<tr>
<td> 470 </td>
<td> (rfc1459) ERR_LINKCHANNEL
</td>
</tr>
<tr>
<td> 471 </td>
<td> (rfc1459) ERR_CHANNELISFULL
</td>
</tr>
<tr>
<td> 472 </td>
<td> (rfc1459) ERR_UNKNOWNMODE
</td>
</tr>
<tr>
<td> 473 </td>
<td> (rfc1459) ERR_INVITEONLYCHAN
</td>
</tr>
<tr>
<td> 474 </td>
<td> (rfc1459) ERR_BANNEDFROMCHAN
</td>
</tr>
<tr>
<td> 475 </td>
<td> (rfc1459) ERR_BADCHANNELKEY
</td>
</tr>
<tr>
<td> 476 </td>
<td> (rfc1459) ERR_BADCHANMASK
</td>
</tr>
<tr>
<td> 477 </td>
<td> (rfc1459) ERR_NEEDREGGEDNICK
</td>
</tr>
<tr>
<td> 478 </td>
<td> (rfc1459) ERR_BANLISTFULL
</td>
</tr>
<tr>
<td> 479 </td>
<td> (rfc1459) ERR_LINKFAIL
</td>
</tr>
<tr>
<td> 480 </td>
<td> (rfc1459) ERR_CANNOTKNOCK
</td>
</tr>
<tr>
<td> 481 </td>
<td> (rfc1459) ERR_NOPRIVILEGES
</td>
</tr>
<tr>
<td> 482 </td>
<td> (rfc1459) ERR_CHANOPRIVSNEEDED
</td>
</tr>
<tr>
<td> 483 </td>
<td> (rfc1459) ERR_CANTKILLSERVER
</td>
</tr>
<tr>
<td> 484 </td>
<td> (rfc1459) ERR_ATTACKDENY
</td>
</tr>
<tr>
<td> 485 </td>
<td> (rfc1459) ERR_KILLDENY
</td>
</tr>
<tr>
<td> 486 </td>
<td> (rfc1459) ERR_HTMDISABLED
</td>
</tr>
<tr>
<td> 491 </td>
<td> (rfc1459) ERR_NOOPERHOST
</td>
</tr>
<tr>
<td> 492 </td>
<td> (rfc1459) ERR_NOSERVICEHOST
</td>
</tr>
<tr>
<td> 501 </td>
<td> (rfc1459) ERR_UMODEUNKNOWNFLAG
</td>
</tr>
<tr>
<td> 502 </td>
<td> (rfc1459) ERR_USERSDONTMATCH
</td>
</tr>
<tr>
<td> 511 </td>
<td> (rfc1459) ERR_SILELISTFULL
</td>
</tr>
<tr>
<td> 512 </td>
<td> (rfc1459) ERR_TOOMANYWATCH
</td>
</tr>
<tr>
<td> 513 </td>
<td> (rfc1459) ERR_NEEDPONG
</td>
</tr>
<tr>
<td> 518 </td>
<td> (rfc1459) ERR_NOINVITE
</td>
</tr>
<tr>
<td> 519 </td>
<td> (rfc1459) ERR_ADMONLY
</td>
</tr>
<tr>
<td> 520 </td>
<td> (rfc1459) ERR_OPERONLY
</td>
</tr>
<tr>
<td> 521 </td>
<td> (rfc1459) ERR_LISTSYNTAX
</td>
</tr>
<tr>
<td> 600 </td>
<td> (rfc1459) RPL_LOGON
</td>
</tr>
<tr>
<td> 601 </td>
<td> (rfc1459) RPL_LOGOFF
</td>
</tr>
<tr>
<td> 602 </td>
<td> (rfc1459) RPL_WATCHOFF
</td>
</tr>
<tr>
<td> 603 </td>
<td> (rfc1459) RPL_WATCHSTAT
</td>
</tr>
<tr>
<td> 604 </td>
<td> (rfc1459) RPL_NOWON
</td>
</tr>
<tr>
<td> 605 </td>
<td> (rfc1459) RPL_NOWOFF
</td>
</tr>
<tr>
<td> 606 </td>
<td> (rfc1459) RPL_WATCHLIST
</td>
</tr>
<tr>
<td> 607 </td>
<td> (rfc1459) RPL_ENDOFWATCHLIST
</td>
</tr>
<tr>
<td> 610 </td>
<td> (rfc1459) RPL_MAPMORE
</td>
</tr>
<tr>
<td> 640 </td>
<td> (rfc1459) RPL_DUMPING
</td>
</tr>
<tr>
<td> 641 </td>
<td> (rfc1459) RPL_DUMPRPL
</td>
</tr>
<tr>
<td> 642 </td>
<td> (rfc1459) RPL_EODUMP
</td>
</tr>
<tr>
<td> 999 </td>
<td> (rfc1459) ERR_NUMERICERR
</td>
</tr>
<tr>
<td> XC_TABOPEN </td>
<td> (xchat) A new channel/nick/server tabs was created.
</td>
</tr>
<tr>
<td> XC_TABCLOSE </td>
<td> (xchat) One of the channel/nick/server tabs was closed.
</td>
</tr>
<tr>
<td> XC_TABFOCUS </td>
<td> (xchat) You changed focus to a new tab.
</td>
</tr>
<tr>
<td> XC_ADDNOTIFY </td>
<td> (xchat) Add Notify
</td>
</tr>
<tr>
<td> XC_BANLIST </td>
<td> (xchat) Ban List
</td>
</tr>
<tr>
<td> XC_BANNED </td>
<td> (xchat) Banned
</td>
</tr>
<tr>
<td> XC_CHANGENICK </td>
<td> (xchat) Change Nick
</td>
</tr>
<tr>
<td> XC_CHANACTION </td>
<td> (xchat) Channel Action
</td>
</tr>
<tr>
<td> XC_HCHANACTION </td>
<td> (xchat) Channel Action Hilight
</td>
</tr>
<tr>
<td> XC_CHANBAN </td>
<td> (xchat) Channel Ban
</td>
</tr>
<tr>
<td> XC_CHANDATE </td>
<td> (xchat) Channel Creation
</td>
</tr>
<tr>
<td> XC_CHANDEHOP </td>
<td> (xchat) Channel DeHalfOp
</td>
</tr>
<tr>
<td> XC_CHANDEOP </td>
<td> (xchat) Channel DeOp
</td>
</tr>
<tr>
<td> XC_CHANDEVOICE </td>
<td> (xchat) Channel DeVoice
</td>
</tr>
<tr>
<td> XC_CHANEXEMPT </td>
<td> (xchat) Channel Exempt
</td>
</tr>
<tr>
<td> XC_CHANHOP </td>
<td> (xchat) Channel Half-Operator
</td>
</tr>
<tr>
<td> XC_CHANINVITE </td>
<td> (xchat) Channel INVITE
</td>
</tr>
<tr>
<td> XC_CHANLISTHEAD </td>
<td> (xchat) Channel List
</td>
</tr>
<tr>
<td> XC_CHANMSG </td>
<td> (xchat) Channel Message
</td>
</tr>
<tr>
<td> XC_CHANMODEGEN </td>
<td> (xchat) Channel Mode Generic
</td>
</tr>
<tr>
<td> XC_CHANMODES </td>
<td> (xchat) Channel Modes
</td>
</tr>
<tr>
<td> XC_HCHANMSG </td>
<td> (xchat) Channel Msg Hilight
</td>
</tr>
<tr>
<td> XC_CHANNOTICE </td>
<td> (xchat) Channel Notice
</td>
</tr>
<tr>
<td> XC_CHANOP </td>
<td> (xchat) Channel Operator
</td>
</tr>
<tr>
<td> XC_CHANRMEXEMPT </td>
<td> (xchat) Channel Remove Exempt
</td>
</tr>
<tr>
<td> XC_CHANRMINVITE </td>
<td> (xchat) Channel Remove Invite
</td>
</tr>
<tr>
<td> XC_CHANRMKEY </td>
<td> (xchat) Channel Remove Keyword
</td>
</tr>
<tr>
<td> XC_CHANRMLIMIT </td>
<td> (xchat) Channel Remove Limit
</td>
</tr>
<tr>
<td> XC_CHANSETKEY </td>
<td> (xchat) Channel Set Key
</td>
</tr>
<tr>
<td> XC_CHANSETLIMIT </td>
<td> (xchat) Channel Set Limit
</td>
</tr>
<tr>
<td> XC_CHANUNBAN </td>
<td> (xchat) Channel UnBan
</td>
</tr>
<tr>
<td> XC_CHANVOICE </td>
<td> (xchat) Channel Voice
</td>
</tr>
<tr>
<td> XC_CONNECTED </td>
<td> (xchat) Connected
</td>
</tr>
<tr>
<td> XC_CONNECT </td>
<td> (xchat) Connecting
</td>
</tr>
<tr>
<td> XC_CONNFAIL </td>
<td> (xchat) Connection Failed
</td>
</tr>
<tr>
<td> XC_CTCPGEN </td>
<td> (xchat) CTCP Generic
</td>
</tr>
<tr>
<td> XC_CTCPGENC </td>
<td> (xchat) CTCP Generic to Channel
</td>
</tr>
<tr>
<td> XC_CTCPSEND </td>
<td> (xchat) CTCP Send
</td>
</tr>
<tr>
<td> XC_CTCPSND </td>
<td> (xchat) CTCP Sound
</td>
</tr>
<tr>
<td> XC_DCCCHATABORT </td>
<td> (xchat) DCC CHAT Abort
</td>
</tr>
<tr>
<td> XC_DCCCONCHAT </td>
<td> (xchat) DCC CHAT Connect
</td>
</tr>
<tr>
<td> XC_DCCCHATF </td>
<td> (xchat) DCC CHAT Failed
</td>
</tr>
<tr>
<td> XC_DCCCHATOFFER </td>
<td> (xchat) DCC CHAT Offer
</td>
</tr>
<tr>
<td> XC_DCCCHATOFFERING </td>
<td> (xchat) DCC CHAT Offering
</td>
</tr>
<tr>
<td> XC_DCCCHATREOFFER </td>
<td> (xchat) DCC CHAT Reoffer
</td>
</tr>
<tr>
<td> XC_DCCCONFAIL </td>
<td> (xchat) DCC Conection Failed
</td>
</tr>
<tr>
<td> XC_DCCGENERICOFFER </td>
<td> (xchat) DCC Generic Offer
</td>
</tr>
<tr>
<td> XC_DCCHEAD </td>
<td> (xchat) DCC Header
</td>
</tr>
<tr>
<td> XC_MALFORMED </td>
<td> (xchat) DCC Malformed
</td>
</tr>
<tr>
<td> XC_DCCOFFER </td>
<td> (xchat) DCC Offer
</td>
</tr>
<tr>
<td> XC_DCCIVAL </td>
<td> (xchat) DCC Offer Not Valid
</td>
</tr>
<tr>
<td> XC_DCCRECVABORT </td>
<td> (xchat) DCC RECV Abort
</td>
</tr>
<tr>
<td> XC_DCCRECVCOMP </td>
<td> (xchat) DCC RECV Complete
</td>
</tr>
<tr>
<td> XC_DCCCONRECV </td>
<td> (xchat) DCC RECV Connect
</td>
</tr>
<tr>
<td> XC_DCCRECVERR </td>
<td> (xchat) DCC RECV Failed
</td>
</tr>
<tr>
<td> XC_DCCFILEERR </td>
<td> (xchat) DCC RECV File Open Error
</td>
</tr>
<tr>
<td> XC_DCCRENAME </td>
<td> (xchat) DCC Rename
</td>
</tr>
<tr>
<td> XC_DCCRESUMEREQUEST </td>
<td> (xchat) DCC RESUME Request
</td>
</tr>
<tr>
<td> XC_DCCSENDABORT </td>
<td> (xchat) DCC SEND Abort
</td>
</tr>
<tr>
<td> XC_DCCSENDCOMP </td>
<td> (xchat) DCC SEND Complete
</td>
</tr>
<tr>
<td> XC_DCCCONSEND </td>
<td> (xchat) DCC SEND Connect
</td>
</tr>
<tr>
<td> XC_DCCSENDFAIL </td>
<td> (xchat) DCC SEND Failed
</td>
</tr>
<tr>
<td> XC_DCCSENDOFFER </td>
<td> (xchat) DCC SEND Offer
</td>
</tr>
<tr>
<td> XC_DCCSTALL </td>
<td> (xchat) DCC Stall
</td>
</tr>
<tr>
<td> XC_DCCTOUT </td>
<td> (xchat) DCC Timeout
</td>
</tr>
<tr>
<td> XC_DELNOTIFY </td>
<td> (xchat) Delete Notify
</td>
</tr>
<tr>
<td> XC_DISCON </td>
<td> (xchat) Disconnected
</td>
</tr>
<tr>
<td> XC_FOUNDIP </td>
<td> (xchat) Found IP
</td>
</tr>
<tr>
<td> XC_IGNOREADD </td>
<td> (xchat) Ignore Add
</td>
</tr>
<tr>
<td> XC_IGNORECHANGE </td>
<td> (xchat) Ignore Changed
</td>
</tr>
<tr>
<td> XC_IGNOREFOOTER </td>
<td> (xchat) Ignore Footer
</td>
</tr>
<tr>
<td> XC_IGNOREHEADER </td>
<td> (xchat) Ignore Header
</td>
</tr>
<tr>
<td> XC_IGNOREREMOVE </td>
<td> (xchat) Ignore Remove
</td>
</tr>
<tr>
<td> XC_IGNOREEMPTY </td>
<td> (xchat) Ignorelist Empty
</td>
</tr>
<tr>
<td> XC_INVITE </td>
<td> (xchat) Invite
</td>
</tr>
<tr>
<td> XC_INVITED </td>
<td> (xchat) Invited
</td>
</tr>
<tr>
<td> XC_JOIN </td>
<td> (xchat) Join
</td>
</tr>
<tr>
<td> XC_KEYPRESS </td>
<td> (xchat) Key Press
</td>
</tr>
<tr>
<td> XC_KEYWORD </td>
<td> (xchat) Keyword
</td>
</tr>
<tr>
<td> XC_KICK </td>
<td> (xchat) Kick
</td>
</tr>
<tr>
<td> XC_KILL </td>
<td> (xchat) Killed
</td>
</tr>
<tr>
<td> XC_MSGSEND </td>
<td> (xchat) Message Send
</td>
</tr>
<tr>
<td> XC_MOTD </td>
<td> (xchat) Motd
</td>
</tr>
<tr>
<td> XC_MOTDSKIP </td>
<td> (xchat) MOTD Skipped
</td>
</tr>
<tr>
<td> XC_NICKCLASH </td>
<td> (xchat) Nick Clash
</td>
</tr>
<tr>
<td> XC_NICKFAIL </td>
<td> (xchat) Nick Failed
</td>
</tr>
<tr>
<td> XC_NODCC </td>
<td> (xchat) No DCC
</td>
</tr>
<tr>
<td> XC_NOCHILD </td>
<td> (xchat) No Running Process
</td>
</tr>
<tr>
<td> XC_NOTICE </td>
<td> (xchat) Notice
</td>
</tr>
<tr>
<td> XC_NOTICESEND </td>
<td> (xchat) Notice Send
</td>
</tr>
<tr>
<td> XC_NOTIFYEMPTY </td>
<td> (xchat) Notify Empty
</td>
</tr>
<tr>
<td> XC_NOTIFYHEAD </td>
<td> (xchat) Notify Header
</td>
</tr>
<tr>
<td> XC_NOTIFYNUMBER </td>
<td> (xchat) Notify Number
</td>
</tr>
<tr>
<td> XC_NOTIFYOFFLINE </td>
<td> (xchat) Notify Offline
</td>
</tr>
<tr>
<td> XC_NOTIFYONLINE </td>
<td> (xchat) Notify Online
</td>
</tr>
<tr>
<td> XC_PART </td>
<td> (xchat) Part
</td>
</tr>
<tr>
<td> XC_PARTREASON </td>
<td> (xchat) Part with Reason
</td>
</tr>
<tr>
<td> XC_PINGREP </td>
<td> (xchat) Ping Reply
</td>
</tr>
<tr>
<td> XC_PINGTIMEOUT </td>
<td> (xchat) Ping Timeout
</td>
</tr>
<tr>
<td> XC_PRIVMSG </td>
<td> (xchat) Private Message
</td>
</tr>
<tr>
<td> XC_DPRIVMSG </td>
<td> (xchat) Private Message to Dialog
</td>
</tr>
<tr>
<td> XC_ALREADYPROCESS </td>
<td> (xchat) Process Already Running
</td>
</tr>
<tr>
<td> XC_QUIT </td>
<td> (xchat) Quit
</td>
</tr>
<tr>
<td> XC_RAWMODES </td>
<td> (xchat) Raw Modes
</td>
</tr>
<tr>
<td> XC_WALLOPS </td>
<td> (xchat) Receive Wallops
</td>
</tr>
<tr>
<td> XC_RESOLVINGUSER </td>
<td> (xchat) Resolving User
</td>
</tr>
<tr>
<td> XC_SERVERCONNECTED </td>
<td> (xchat) Server Connected
</td>
</tr>
<tr>
<td> XC_SERVERERROR </td>
<td> (xchat) Server Error
</td>
</tr>
<tr>
<td> XC_SERVERLOOKUP </td>
<td> (xchat) Server Lookup
</td>
</tr>
<tr>
<td> XC_SERVNOTICE </td>
<td> (xchat) Server Notice
</td>
</tr>
<tr>
<td> XC_SERVTEXT </td>
<td> (xchat) Server Text
</td>
</tr>
<tr>
<td> XC_STOPCONNECT </td>
<td> (xchat) Stop Connection
</td>
</tr>
<tr>
<td> XC_TOPIC </td>
<td> (xchat) Topic
</td>
</tr>
<tr>
<td> XC_TOPICDATE </td>
<td> (xchat) Topic Creation
</td>
</tr>
<tr>
<td> XC_NEWTOPIC </td>
<td> (xchat) Topic Change
</td>
</tr>
<tr>
<td> XC_UKNHOST </td>
<td> (xchat) Unknown Host
</td>
</tr>
<tr>
<td> XC_USERLIMIT </td>
<td> (xchat) User Limit
</td>
</tr>
<tr>
<td> XC_USERSONCHAN </td>
<td> (xchat) Users On Channel
</td>
</tr>
<tr>
<td> XC_WHOIS5 </td>
<td> (xchat) WhoIs Away Line
</td>
</tr>
<tr>
<td> XC_WHOIS2 </td>
<td> (xchat) WhoIs Channel/Oper Line
</td>
</tr>
<tr>
<td> XC_WHOIS6 </td>
<td> (xchat) WhoIs End
</td>
</tr>
<tr>
<td> XC_WHOIS4 </td>
<td> (xchat) WhoIs Idle Line
</td>
</tr>
<tr>
<td> XC_WHOIS4T </td>
<td> (xchat) WhoIs Idle Line with Signon
</td>
</tr>
<tr>
<td> XC_WHOIS1 </td>
<td> (xchat) WhoIs Name Line
</td>
</tr>
<tr>
<td> XC_WHOIS3 </td>
<td> (xchat) WhoIs Server Line
</td>
</tr>
<tr>
<td> XC_UJOIN </td>
<td> (xchat) You Join
</td>
</tr>
<tr>
<td> XC_UPART </td>
<td> (xchat) You Part
</td>
</tr>
<tr>
<td> XC_UPARTREASON </td>
<td> (xchat) You Part with Reason
</td>
</tr>
<tr>
<td> XC_UKICK </td>
<td> (xchat) You Kicked
</td>
</tr>
<tr>
<td> XC_UINVITE </td>
<td> (xchat) Your Invitation
</td>
</tr>
<tr>
<td> XC_UCHANMSG </td>
<td> (xchat) Your Message
</td>
</tr>
<tr>
<td> XC_UCHANGENICK </td>
<td> (xchat) Your Nick Changing</td>
</tr>
</table><br>
</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>on PRIVMSG example {
  if { [string match -nocase "*[me]*" $_rest] } {
    play mynick.wav
    complete
  }
}

on !opme example {
  splitsrc
  /op $_nick
  complete
}

on XC_TABOPEN example {
  switch [string index [channel] 0] {
    "#" -
    "&" -
    "(" -
    "" { return }
  }
  play attention.wav
  print "Now in private conversation with [channel]."
  complete
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>Notes:</b></td>
<td>All events starting with <b><i>XC_</i></b> correspond to the events listed in the <b><i>Settings->Lists->EventTexts</i></b> window in XChat. All parameters are appended to <b><i>$_raw</i></b>, e.g:
<br>
<br> arg1 is [lindex $_raw 1]
<br> arg2 is [lindex $_raw 2]
<br> arg3 is [lindex $_raw 3]
<br> arg4 is [lindex $_raw 4]</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#alias'>alias</a>, <a href='#off'>off</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='print'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>print - Print text to an xchat window/tab</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>print ?server|context? ?channel|nick? text</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Prints text to a window.  If a <i>channel|nick</i> is included, the text is printed to that channel/nick.  You may also include a specific server.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre># print text to the current window
print "Hello, World!"

# print text to the channel or nick window
print #channel "Hello, World!"

# print text to the channel window
# belonging to a specific server.
print irc.blahblah.com #channel "Hello, World!"</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#puts'>puts</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='queries'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>queries - Returns a list of private queries.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>queries ?server|context?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns a list of all private queries. If <b><i>server</i></b> is omitted, the server belonging to the current server is used by default.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>alias myqueries {
  foreach s [servers] {
    print "Server: $s"
    foreach q [queries $s] {
      print " - Query: $q"
    }
  }
  complete
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#channels'>channels</a>, <a href='#chats'>chats</a>, <a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='raw'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>raw - Send a line directly to the server.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>raw ?server|context? ?channel|nick? text</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>This command sends <i>text</i> directly to the server without further processing or interpretation by xchat.  If <b><i>server</i></b> or <b><i>channel|nick</i></b> name is omitted, the current ones are used by default.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>raw "PRIVMSG bubba :Howdy Bubba!"</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#command'>command</a>, <a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='server'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>server - Return the current server.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>server ?context?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the current server name (what the server claims to be).</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>print "I attempted to connect to [host] on [network]."
print "I am actually connected to [server]."</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#host'>host</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='servers'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>servers - Returns of list of all servers you are on.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>servers</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns a list of all servers you are currently connected to.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>alias mychannels {
  foreach s [servers] {
    print "Server: $s"
    foreach c [channels $s] {
      print " - Channel: $c - [topic $s $c]"
    }
  }
  complete
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#channel'>channel</a>, <a href='#channels'>channels</a>, <a href='#server'>server</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='setcontext'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>setcontext - Changes your current context to the one given.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>setcontext context</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Changes your current context to the one given.  The argument <i>context</i> must have been returned by <b><i>getcontext</i></b> or <b><i>findcontext</i></b>.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>set context [findcontext #channel]
setcontext $context</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>Notes:</b></td>
<td>This function is not normally needed with the tclplugin.  It is included only to add completeness with the XChat C API.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='timer'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>timer - Executes tcl command after a certain number of seconds have passed.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>timer ?-repeat? ?-count times? seconds {script | procname ?args?}</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Executes a tcl command or script after a certain number of seconds have passed.
<br>

<br>
If the <b><i>-repeat</i></b> flag is included, it will will keep repeating until killed with <b><i>killtimer</i></b>.  If the <b><i>-count</i></b> flag is added, it will repeat the number of times specified after the flag.  In all other cases, it is executed only once.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>timer 5 { /say Times up! }</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Returns:</b></td>
<td>timer ID code is to identify the timer with for use with other timer commands.</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#killtimer'>killtimer</a>, <a href='#timerexists'>timerexists</a>, <a href='#timers'>timers</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='timerexists'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>timerexists - Returns 1 if the specified timer exists.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>timerexists timerID</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Determines of the specified timerID exists.</td>
</tr>

<tr valign=top>
<td align=right><b>Returns:</b></td>
<td>1 if the specified timer exists, 0 otherwise</td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#killtimer'>killtimer</a>, <a href='#timer'>timer</a>, <a href='#timers'>timers</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='timers'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>timers - Returns a list of timers currently active.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>timers</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns a list of active timers; each entry in the list contains the timerID, the number of seconds left till activation, the command that will be executed, the number of seconds specified, and the number of times left to be executed.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>timer 5 { print "Important message coming soon!" }
timer 10 { print "It is now 10 seconds later!  Yay!!!!!" }
print "[timers]"</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#killtimer'>killtimer</a>, <a href='#timer'>timer</a>, <a href='#timerexists'>timerexists</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='topic'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>topic - Returns the topic of a channel.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>topic ?server|context? ?channel?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the channel topic from the current channel or from a specific server and channel.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>alias mychannels {
  foreach s [servers] {
    print "Server: $s"
    foreach c [channels $s] {
      print " - Channel: $c - [topic $s $c]"
    }
  }
  complete
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#channel'>channel</a>, <a href='#channels'>channels</a>, <a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#users'>users</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='users'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>users - Returns a list of users in a channel.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>users ?server|context? ?channel?</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns a list of all the users in a channel.  The list consists of 4 elements; nick, hostmask, channel status and selected.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>alias listusers {
  print "- --------------- ----------------------------------------"
  foreach user [users] {
    print "[format "%-1s" [lindex $user 2]] [format "%-15s" [lindex $user 0]] [lindex $user 1]"
  }
}</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#channels'>channels</a>, <a href='#findcontext'>findcontext</a>, <a href='#getcontext'>getcontext</a>, <a href='#getlist'>getlist</a>, <a href='#servers'>servers</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='version'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>version - Returns XChat  version number.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>version</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the full XChat version number.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>print "I am using XChat version [version]"</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#xchatdir'>xchatdir</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>


<a name='xchatdir'> </a>
<table width=100% border=0 bgcolor=#eeeeee cellpadding=3 cellspacing=0>

<tr valign=top>
<td align=right width=1% ><b>Name:</b></td>
<td>xchatdir - Returns the current xchat config directory.</td>
</tr>

<tr valign=top>
<td align=right><b>Synopsis:</b></td>
<td><pre>xchatdir</pre></td>
</tr>

<tr valign=top>
<td align=right><b>Description:</b></td>
<td>Returns the current xchat config dir within your own user space.</td>
</tr>

<tr valign=top>
<td align=right><b>Example:</b></td>
<td><pre>print "My XChat config directory is [xchatdir]"</pre></td>
</tr>

<tr valign=top>
<td align=right nowrap><b>See Also:</b></td>
<td><a href='#version'>version</a></td>
</tr>


<tr valign=top>
<td align=right nowrap><b>Downloads:</b></td>
<td><a href='http://www.scriptkitties.com/tclplugin/pluginscripts.tar.gz'>Download recommended Tcl plugin support scripts.</a></td>
</tr>

</table>
<p>
