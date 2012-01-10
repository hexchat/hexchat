/* XChat-WDK
 * Copyright (c) 2010 <ygrek@autistici.org>
 * Copyright (c) 2012 Berke Viktor.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * SASL authentication plugin for XChat
 * Extremely primitive: only PLAIN, no error checking
 *
 * http://ygrek.org.ua/p/cap_sasl.html
 *
 * Docs:
 *  http://hg.atheme.org/charybdis/charybdis/file/6144f52a119b/doc/sasl.txt
 *  http://tools.ietf.org/html/rfc4422
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <glib/gbase64.h>

#include "xchat-plugin.h"

static xchat_plugin *ph;   /* plugin handle */
static const char name[] = "XSASL";
static const char desc[] = "SASL authentication plugin for XChat";
static const char version[] = "1.0";

struct sasl_info
{
	char const* login;
	char const* password;
	char const* network;
};

typedef struct sasl_info sasl_info;

static void
add_info (char const* login, char const* password, char const* network)
{
	char buffer[512];

	sprintf (buffer, "%s:%s", login, password);
	xchat_set_pluginpref_str (ph, network, buffer);
}

static sasl_info*
find_info (char const* network)
{
	char buffer[512];
	char* token;
	sasl_info* cur = (sasl_info*) malloc (sizeof (sasl_info));

	if (xchat_get_pluginpref_str (ph, network, buffer))
	{
		token = strtok (buffer, ":");
		cur->login = g_strdup (token);
		token = strtok (NULL, ":");
		cur->password = g_strdup (token);
		cur->network = g_strdup (network);

		return cur;
	}

	return NULL;
}

static sasl_info*
get_info (void)
{
	const char* name;
	name = xchat_get_info (ph, "network");

	if (name)
	{
		return find_info (name);
	}
	else
	{
		return NULL;
	}
}

static int
authend_cb (char *word[], char *word_eol[], void *userdata)
{
	if (get_info ())
	{
		xchat_printf (ph, "XSASL result: %s\n", word_eol[1]);
		xchat_commandf (ph, "QUOTE CAP END");
	}

	return XCHAT_EAT_ALL;
}

/*
static int
disconnect_cb (char *word[], void *userdata)
{
	xchat_printf (ph, "disconnected\n");
	return XCHAT_EAT_NONE;
}
*/

static int
server_cb (char *word[], char *word_eol[], void *userdata)
{
	size_t len;
	char* buf;
	char* enc;
	sasl_info* p;

	if (strcmp ("AUTHENTICATE", word[1]) == 0 && strcmp ("+", word[2]) == 0)
	{
		p = get_info ();

		if (!p)
		{
			return XCHAT_EAT_NONE;
		}

		xchat_printf (ph, "XSASL authenticating as %s\n", p->login);

		len = strlen (p->login) * 2 + 2 + strlen (p->password);
		buf = (char*) malloc (len + 1);
		strcpy (buf, p->login);
		strcpy (buf + strlen (p->login) + 1, p->login);
		strcpy (buf + strlen (p->login) * 2 + 2, p->password);
		enc = g_base64_encode ((unsigned char*) buf, len);

		/* xchat_printf (ph, "AUTHENTICATE %s\}", enc); */
		xchat_commandf (ph, "QUOTE AUTHENTICATE %s", enc);

		free (enc);
		free (buf);

		return XCHAT_EAT_ALL;
	}

	return XCHAT_EAT_NONE;
}

static int
cap_cb (char *word[], char *word_eol[], void *userdata)
{
	if (get_info ())
	{
		/* FIXME test sasl cap */
		xchat_printf (ph, "XSASL info: %s\n", word_eol[1]);
		xchat_commandf (ph, "QUOTE AUTHENTICATE PLAIN");
	}

	return XCHAT_EAT_ALL;
}

static int
sasl_cmd_cb (char *word[], char *word_eol[], void *userdata)
{
	const char* login = word[2];
	const char* password = word[3];
	const char* network = word_eol[4];

	if (!login || !password || !network || !*login || !*password || !*network)
	{
		xchat_printf (ph, "Usage: XSASL <login> <password> <network>, enable SASL authentication for given network\n");
		return XCHAT_EAT_ALL;
	}

	add_info (login, password, network);
	xchat_printf (ph, "Enabled SASL authentication for the \"%s\" network\n", network);

	return XCHAT_EAT_ALL;
}

static int
connect_cb (char *word[], void *userdata)
{
	if (get_info ())
	{
		xchat_printf (ph, "XSASL enabled\n");
		xchat_commandf (ph, "QUOTE CAP REQ :sasl");
	}

	return XCHAT_EAT_NONE;
}

int
xchat_plugin_init (xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	/* we need to save this for use with any xchat_* functions */
	ph = plugin_handle;

	/* tell xchat our info */
	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	xchat_hook_command (ph, "XSASL", XCHAT_PRI_NORM, sasl_cmd_cb, "Usage: XSASL <login> <password> <network>, enable SASL authentication for given network", 0);
	xchat_hook_print (ph, "Connected", XCHAT_PRI_NORM, connect_cb, NULL);
	/* xchat_hook_print (ph, "Disconnected", XCHAT_PRI_NORM, disconnect_cb, NULL); */
	xchat_hook_server (ph, "CAP", XCHAT_PRI_NORM, cap_cb, NULL);
	xchat_hook_server (ph, "RAW LINE", XCHAT_PRI_NORM, server_cb, NULL);
	xchat_hook_server (ph, "903", XCHAT_PRI_NORM, authend_cb, NULL);
	xchat_hook_server (ph, "904", XCHAT_PRI_NORM, authend_cb, NULL);
	xchat_hook_server (ph, "905", XCHAT_PRI_NORM, authend_cb, NULL);
	xchat_hook_server (ph, "906", XCHAT_PRI_NORM, authend_cb, NULL);
	xchat_hook_server (ph, "907", XCHAT_PRI_NORM, authend_cb, NULL);

	xchat_printf (ph, "%s plugin loaded\n", name);

	return 1;
}

int
xchat_plugin_deinit (void)
{
	xchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
