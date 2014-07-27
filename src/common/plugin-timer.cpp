/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include "hexchat-plugin.h"

#ifdef WIN32
#define g_ascii_strcasecmp stricmp
#endif

#define _(x) hexchat_gettext(ph,x)

namespace {
static hexchat_plugin *ph;	/* plugin handle */

#define STATIC
#define HELP \
"Usage: TIMER [-refnum <num>] [-repeat <num>] <seconds> <command>\n" \
"       TIMER [-quiet] -delete <num>"

struct timer;
static int
timeout_cb(timer &tim);

struct timer
{
	std::string command;
	int ref;
	int repeat;
	float timeout;
	bool forever;
	hexchat_context *context;
	hexchat_hook *hook;

	timer(const std::string& command, int ref, int repeat, float timeout, hexchat_context* context)
		:command(command), ref(ref), repeat(repeat), timeout(timeout), forever(repeat == 0), context(context)
	{
		this->hook = hexchat_hook_timer(ph, static_cast<int>(timeout * 1000.0f), (int(*) (void *))timeout_cb, this);
	}
	~timer()
	{
		hexchat_unhook(ph, this->hook);
	}
};
	

static ::std::unordered_map<int, std::shared_ptr<timer> > timer_map;

static void
timer_del_ref (int ref, bool quiet)
{
	if (timer_map.erase(ref))
	{
		if (!quiet)
			hexchat_printf(ph, _("Timer %d deleted.\n"), ref);
		return;
	}

	if (!quiet)
		hexchat_print (ph, _("No such ref number found.\n"));
}

static int
timeout_cb (timer &tim)
{
	if (hexchat_set_context (ph, tim.context))
	{
		hexchat_command (ph, tim.command.c_str());

		if (tim.forever)
			return 1;

		tim.repeat--;
		if (tim.repeat > 0)
			return 1;
	}

	timer_map.erase(tim.ref);
	return 0;
}

static void
timer_add (int ref, float timeout, int repeat, const std::string & command)
{
	if (ref == 0)
	{
		ref = 1;

		for (const auto & bucket : timer_map)
		{
			ref = std::max(bucket.second->ref, ref + 1);
		}
	}

	timer_map.emplace(ref, std::make_shared<timer>(command, ref, repeat, timeout, hexchat_get_context(ph)));
}

static void
timer_showlist (void)
{
	if (timer_map.empty())
	{
		hexchat_print (ph, _("No timers installed.\n"));
		hexchat_print (ph, _(HELP));
		return;
	}
							 /*  00000 00000000 0000000 abc */
	hexchat_print (ph, _("\026 Ref#  Seconds  Repeat  Command \026\n"));

	for (const auto & bucket: timer_map)
	{
		hexchat_printf(ph, _("%5d %8.1f %7d  %s\n"), bucket.second->ref, bucket.second->timeout,
			bucket.second->repeat, bucket.second->command.c_str());
	}
}

static int
timer_cb (char *word[], char *word_eol[], void *userdata)
{
	int repeat = 1;
	float timeout;
	int offset = 0;
	int ref = 0;
	bool quiet = false;
	char *command;

	if (!word[2][0])
	{
		timer_showlist ();
		return HEXCHAT_EAT_HEXCHAT;
	}

	if (g_ascii_strcasecmp (word[2], "-quiet") == 0)
	{
		quiet = true;
		offset++;
	}

	if (g_ascii_strcasecmp (word[2 + offset], "-delete") == 0)
	{
		timer_del_ref (atoi (word[3 + offset]), quiet);
		return HEXCHAT_EAT_HEXCHAT;
	}

	if (g_ascii_strcasecmp (word[2 + offset], "-refnum") == 0)
	{
		ref = atoi (word[3 + offset]);
		offset += 2;
	}

	if (g_ascii_strcasecmp (word[2 + offset], "-repeat") == 0)
	{
		repeat = atoi (word[3 + offset]);
		offset += 2;
	}

	timeout = std::strtof(word[2 + offset], nullptr);
	command = word_eol[3 + offset];

	if (timeout < 0.1 || !command[0])
		hexchat_print (ph, HELP);
	else
		timer_add (ref, timeout, repeat, command);

	return HEXCHAT_EAT_HEXCHAT;
}
}

int
#ifdef STATIC
timer_plugin_init
#else
hexchat_plugin_init
#endif
				(hexchat_plugin *plugin_handle, char **plugin_name,
				char **plugin_desc, char **plugin_version, char *arg)
{
	/* we need to save this for use with any hexchat_* functions */
	ph = plugin_handle;

	*plugin_name = "Timer";
	*plugin_desc = "IrcII style /TIMER command";
	*plugin_version = "";

	hexchat_hook_command (ph, "TIMER", HEXCHAT_PRI_NORM, timer_cb, _(HELP), 0);

	return 1;       /* return 1 for success */
}

int
#ifdef STATIC
timer_plugin_deinit(void)
#else
hexchat_plugin_deinit(void)
#endif
{
	timer_map.clear();
	return 1;
}