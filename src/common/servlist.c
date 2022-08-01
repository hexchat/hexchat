/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "hexchat.h"
#include "cfgfiles.h"
#include "fe.h"
#include "server.h"
#include "text.h"
#include "util.h" /* token_foreach */
#include "hexchatc.h"

#include "servlist.h"


struct defaultserver
{
	char *network;
	char *host;
	char *channel;
	char *charset;
	int loginmode;		/* default authentication type */
	char *connectcmd;	/* default connect command - should only be used for rare login types, paired with LOGIN_CUSTOM */
	gboolean ssl;
};

static const struct defaultserver def[] =
{
	{"2600net",	0},
	/* Invalid hostname in cert */
	{0,			"irc.2600.net"},


	{"AfterNET", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.afternet.org"},

	{"Aitvaras",	0},
#ifdef USE_OPENSSL
	{0,			"irc.data.lt/+6668"},
	{0,			"irc.omicron.lt/+6668"},
	{0,			"irc.vub.lt/+6668"},
#endif
	{0,			"irc.data.lt"},
	{0,			"irc.omicron.lt"},
	{0,			"irc.vub.lt"},

	{"Anthrochat", 0, 0, 0, 0, 0, TRUE},
	{0,			"irc.anthrochat.net"},

	{"ARCNet",	0},
	{0,			"arcnet-irc.org"},

	{"AustNet",	0},
	{0,			"irc.austnet.org"},

	{"AzzurraNet",	0},
	{0,			"irc.azzurra.org"},

	{"Canternet", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.canternet.org"},

	{"Chat4all", 0, 0, 0, 0, 0, TRUE},
	{0,			"irc.chat4all.org"},

	{"ChatJunkies",	0},
	{0,			"irc.chatjunkies.org"},

	{"chatpat", 0, 0, "CP1251", LOGIN_CUSTOM, "MSG NS IDENTIFY %p"},
	{0,			"irc.unibg.net"},
	{0,			"irc.chatpat.bg"},

	{"ChatSpike", 0, 0, 0, LOGIN_SASL},
	{0,			"irc.chatspike.net"},

	{"DaIRC", 0},
	{0,			"irc.dairc.net"},
	
	{"DALnet", 0, 0, 0, LOGIN_NICKSERV},
	/* Self signed */
	{0,			"us.dal.net"},

	{"DarkMyst", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.darkmyst.org"},

#ifdef USE_OPENSSL
	{"darkscience", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.darkscience.net"},
	{0,			"irc.drk.sc"},
	{0,			"irc.darkscience.ws"},
#endif

	{"Dark-Tou-Net",	0},
	{0,			"irc.d-t-net.de"},
	
	{"DigitalIRC", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.digitalirc.org"},
	
#ifdef USE_OPENSSL
	{"DosersNET", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.dosers.net/+6697"},
#endif

	{"EFnet",	0},
	{0,			"irc.choopa.net"},
	{0,			"efnet.port80.se"},
	{0,			"irc.underworld.no"},
	{0,			"efnet.deic.eu"},

	{"EnterTheGame",	0},
	{0,			"irc.enterthegame.com"},

	{"EntropyNet",	0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.entropynet.net"},

	{"EsperNet", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.esper.net"},

	{"EUIrc",	0},
	{0,			"irc.euirc.net"},

	{"EuropNet", 0},
	/* Self signed */
	{0,			"irc.europnet.org"},

	{"FDFNet",	0},
	/* Self signed */
	{0,			"irc.fdfnet.net"},

	{"GameSurge", 0},
	{0,			"irc.gamesurge.net"},

	{"GeekShed", 0, 0, 0, 0, 0, TRUE},
	{0,			"irc.geekshed.net"},

	{"German-Elite", 0, 0, "CP1252"},
	{0,			"irc.german-elite.net"},

	{"GIMPNet",		0},
	/* Invalid hostname in cert */
	{0,			"irc.gimp.org"},
	{0,			"irc.gnome.org"},

	{"GlobalGamers", 0},
#ifdef USE_OPENSSL
	{0,			"irc.globalgamers.net/+6660"},
#endif
	{0,			"irc.globalgamers.net"},

#ifdef USE_OPENSSL
	{"hackint", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.hackint.org"},
	{0,			"irc.eu.hackint.org"},
#endif

	{"Hashmark",	0},
	{0,			"irc.hashmark.net"},

	{"ICQ-Chat", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.icq-chat.com"},

	{"Interlinked", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.interlinked.me"},

	{"Irc-Nerds", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.irc-nerds.net"},
	
	{"IRC4Fun", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,				"irc.irc4fun.net"},

	{"IRCNet",		0},
	{0,				"open.ircnet.net"},

	{"IRCtoo",	0},
	{0,			"irc.irctoo.net"},

	{"Keyboard-Failure", 0},
	/* SSL is self-signed */
	{0,			"irc.kbfail.net"},

	{"Libera.Chat", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.libera.chat"},
	
#ifdef USE_OPENSSL
	{"LibertaCasa", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.liberta.casa"},
#endif

	{"LibraIRC", 0},
	/* Self signed */
	{0,			"irc.librairc.net"},

#ifdef USE_OPENSSL
	{"LinkNet",	0},
	{0,			"irc.link-net.org/+7000"},
#endif

	{"MindForge", 0, 0, 0, LOGIN_SASL},
	{0,			"irc.mindforge.org"},

	{"MIXXnet",		0},
	{0,			"irc.mixxnet.net"},

	{"Oceanius", 0, 0, 0, LOGIN_SASL},
	/* Self signed */
	{0,			"irc.oceanius.com"},

	{"OFTC", 0, 0, 0, 0, 0, TRUE},
	{0,			"irc.oftc.net"},

	{"OtherNet",	0},
	{0,			"irc.othernet.org"},

	{"OzOrg",	0},
	{0,			"irc.oz.org"},

	{"PIK", 0},
	{0,			"irc.krstarica.com"},

	{"pirc.pl",	0, 0, 0, 0, 0, TRUE},
	{0,			"irc.pirc.pl"},

	{"PTNet",	0},
	{0,			"irc.ptnet.org"},
	{0,			"uevora.ptnet.org"},
	{0,			"claranet.ptnet.org"},
	{0,			"sonaquela.ptnet.org"},
	{0,			"uc.ptnet.org"},
	{0,			"ipg.ptnet.org"},

	{"QuakeNet", 0, 0, 0, LOGIN_CHALLENGEAUTH},
	{0,			"irc.quakenet.org"},

	{"Rizon", 0, 0, 0, 0, 0, TRUE},
	{0,			"irc.rizon.net"},

	{"RusNet", 0, 0, "KOI8-R (Cyrillic)"},
	/* Self signed */
	{0,			"irc.tomsk.net"},
	{0,			"irc.run.net"},
	{0,			"irc.ru"},
	{0,			"irc.lucky.net"},

	{"Serenity-IRC",	0},
	{0,			"irc.serenity-irc.net"},

	{"SimosNap", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,            "irc.simosnap.com"},

	{"SlashNET",	0},
	/* Self signed */
	{0,			"irc.slashnet.org"},

	{"Snoonet", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.snoonet.org"},

	{"Sohbet.Net", 0, 0, "CP1254"},
	{0,			"irc.sohbet.net"},

	{"SorceryNet", 0, 0, 0, LOGIN_SASL},
	/* Self signed */
	{0,			"irc.sorcery.net"},
	
	{"SpotChat", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.spotchat.org"},

	{"Station51", 0},
	/* Self signed */
	{0,			"irc.station51.net"},

	{"StormBit", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.stormbit.net"},

	{"SwiftIRC", 0},
	/* Expired cert */
	{0,			"irc.swiftirc.net"},

	{"synIRC", 0},
	/* Self signed */
	{0, "irc.synirc.net"},

	{"Techtronix",	0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.techtronix.net"},
	
	{"tilde.chat", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.tilde.chat"},

	{"TURLINet", 0, 0, 0, 0, 0, TRUE},
	/* all servers use UTF-8 and valid certs */
	{0,			"irc.servx.org"},
	{0,			"i.valware.uk"},
	
	
#ifdef USE_OPENSSL
	{"TripSit", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.tripsit.me"},
	{0,			"newirc.tripsit.me"},
	{0,			"coconut.tripsit.me"},
	{0,			"innsbruck.tripsit.me"},
#endif	

	{"UnderNet", 0, 0, 0, LOGIN_CUSTOM, "MSG x@channels.undernet.org login %u %p"},
	{0,			"us.undernet.org"},

	{"Xertion", 0, 0, 0, LOGIN_SASL, 0, TRUE},
	{0,			"irc.xertion.org"},

	{0,0}
};

GSList *network_list = 0;

favchannel *
servlist_favchan_copy (favchannel *fav)
{
	favchannel *newfav;

	newfav = g_new (favchannel, 1);
	newfav->name = g_strdup (fav->name);
	newfav->key = g_strdup (fav->key);		/* g_strdup() can handle NULLs so no need to check it */

	return newfav;
}

void
servlist_connect (session *sess, ircnet *net, gboolean join)
{
	ircserver *ircserv;
	GSList *list;
	char *port;
	server *serv;

	if (!sess)
		sess = new_ircwindow (NULL, NULL, SESS_SERVER, TRUE);

	serv = sess->server;

	/* connect to the currently selected Server-row */
	list = g_slist_nth (net->servlist, net->selected);
	if (!list)
		list = net->servlist;
	if (!list)
		return;
	ircserv = list->data;

	/* in case a protocol switch is added to the servlist gui */
	server_fill_her_up (sess->server);

	if (join)
	{
		sess->willjoinchannel[0] = 0;

		if (net->favchanlist)
		{
			if (serv->favlist)
			{
				g_slist_free_full (serv->favlist, (GDestroyNotify) servlist_favchan_free);
			}
			serv->favlist = g_slist_copy_deep (net->favchanlist, (GCopyFunc) servlist_favchan_copy, NULL);
		}
	}

	if (net->logintype)
	{
		serv->loginmethod = net->logintype;
	}
	else
	{
		serv->loginmethod = LOGIN_DEFAULT_REAL;
	}

	serv->password[0] = 0;

	if (net->pass)
	{
		safe_strcpy (serv->password, net->pass, sizeof (serv->password));
	}

	if (net->flags & FLAG_USE_GLOBAL)
	{
		strcpy (serv->nick, prefs.hex_irc_nick1);
	}
	else
	{
		if (net->nick)
			strcpy (serv->nick, net->nick);
	}

	serv->dont_use_proxy = (net->flags & FLAG_USE_PROXY) ? FALSE : TRUE;

#ifdef USE_OPENSSL
	serv->use_ssl = (net->flags & FLAG_USE_SSL) ? TRUE : FALSE;
	serv->accept_invalid_cert =
		(net->flags & FLAG_ALLOW_INVALID) ? TRUE : FALSE;
#endif

	serv->network = net;

	port = strrchr (ircserv->hostname, '/');
	if (port)
	{
		*port = 0;

		/* support "+port" to indicate SSL (like mIRC does) */
		if (port[1] == '+')
		{
#ifdef USE_OPENSSL
			serv->use_ssl = TRUE;
#endif
			serv->connect (serv, ircserv->hostname, atoi (port + 2), FALSE);
		} else
		{
			serv->connect (serv, ircserv->hostname, atoi (port + 1), FALSE);
		}

		*port = '/';
	} else
		serv->connect (serv, ircserv->hostname, -1, FALSE);

	server_set_encoding (serv, net->encoding);
}

int
servlist_connect_by_netname (session *sess, char *network, gboolean join)
{
	ircnet *net;
	GSList *list = network_list;

	while (list)
	{
		net = list->data;

		if (g_ascii_strcasecmp (net->name, network) == 0)
		{
			servlist_connect (sess, net, join);
			return 1;
		}

		list = list->next;
	}

	return 0;
}

int
servlist_have_auto (void)
{
	GSList *list = network_list;
	ircnet *net;

	while (list)
	{
		net = list->data;

		if (net->flags & FLAG_AUTO_CONNECT)
			return 1;

		list = list->next;
	}

	return 0;
}

int
servlist_auto_connect (session *sess)
{
	GSList *list = network_list;
	ircnet *net;
	int ret = 0;

	while (list)
	{
		net = list->data;

		if (net->flags & FLAG_AUTO_CONNECT)
		{
			servlist_connect (sess, net, TRUE);
			ret = 1;
		}

		list = list->next;
	}

	return ret;
}

static gint
servlist_cycle_cb (server *serv)
{
	if (serv->network)
	{
		PrintTextf (serv->server_session,
			_("Cycling to next server in %s...\n"), ((ircnet *)serv->network)->name);
		servlist_connect (serv->server_session, serv->network, TRUE);
	}

	return 0;
}

int
servlist_cycle (server *serv)
{
	ircnet *net;
	int max, del;

	net = serv->network;
	if (net)
	{
		max = g_slist_length (net->servlist);
		if (max > 0)
		{
			/* try the next server, if that option is on */
			if (net->flags & FLAG_CYCLE)
			{
				net->selected++;
				if (net->selected >= max)
					net->selected = 0;
			}

			del = prefs.hex_net_reconnect_delay * 1000;
			if (del < 1000)
				del = 500;				  /* so it doesn't block the gui */

			if (del)
				serv->recondelay_tag = fe_timeout_add (del, servlist_cycle_cb, serv);
			else
				servlist_connect (serv->server_session, net, TRUE);

			return TRUE;
		}
	}

	return FALSE;
}

ircserver *
servlist_server_find (ircnet *net, char *name, int *pos)
{
	GSList *list = net->servlist;
	ircserver *serv;
	int i = 0;

	while (list)
	{
		serv = list->data;
		if (strcmp (serv->hostname, name) == 0)
		{
			if (pos)
			{
				*pos = i;
			}
			return serv;
		}
		i++;
		list = list->next;
	}

	return NULL;
}

favchannel *
servlist_favchan_find (ircnet *net, char *channel, int *pos)
{
	GSList *list;
	favchannel *favchan;
	int i = 0;

	if (net == NULL)
		return NULL;

	list = net->favchanlist;

	while (list)
	{
		favchan = list->data;
		if (g_ascii_strcasecmp (favchan->name, channel) == 0)
		{
			if (pos)
			{
				*pos = i;
			}
			return favchan;
		}
		i++;
		list = list->next;
	}

	return NULL;
}

commandentry *
servlist_command_find (ircnet *net, char *cmd, int *pos)
{
	GSList *list = net->commandlist;
	commandentry *entry;
	int i = 0;

	while (list)
	{
		entry = list->data;
		if (strcmp (entry->command, cmd) == 0)
		{
			if (pos)
			{
				*pos = i;
			}
			return entry;
		}
		i++;
		list = list->next;
	}

	return NULL;
}

/* find a network (e.g. (ircnet *) to "FreeNode") from a hostname
   (e.g. "irc.eu.freenode.net") */

ircnet *
servlist_net_find_from_server (char *server_name)
{
	GSList *list = network_list;
	GSList *slist;
	ircnet *net;
	ircserver *serv;

	while (list)
	{
		net = list->data;

		slist = net->servlist;
		while (slist)
		{
			gsize hostname_len;
			const char *hostname, *p;

			serv = slist->data;
			hostname = serv->hostname;

			/* Ignore port when comparing */
			if ((p = strchr (hostname, '/')))
				hostname_len = p - hostname;
			else
				hostname_len = strlen (hostname);

			if (g_ascii_strncasecmp (hostname, server_name, hostname_len) == 0)
				return net;
			slist = slist->next;
		}

		list = list->next;
	}

	return NULL;
}

ircnet *
servlist_net_find (char *name, int *pos, int (*cmpfunc) (const char *, const char *))
{
	GSList *list = network_list;
	ircnet *net;
	int i = 0;

	while (list)
	{
		net = list->data;
		if (cmpfunc (net->name, name) == 0)
		{
			if (pos)
				*pos = i;
			return net;
		}
		i++;
		list = list->next;
	}

	return NULL;
}

ircserver *
servlist_server_add (ircnet *net, char *name)
{
	ircserver *serv;

	serv = g_new (ircserver, 1);
	serv->hostname = g_strdup (name);

	net->servlist = g_slist_append (net->servlist, serv);

	return serv;
}

commandentry *
servlist_command_add (ircnet *net, char *cmd)
{
	commandentry *entry;

	entry = g_new (commandentry, 1);
	entry->command = g_strdup (cmd);

	net->commandlist = g_slist_append (net->commandlist, entry);

	return entry;
}

GSList *
servlist_favchan_listadd (GSList *chanlist, char *channel, char *key)
{
	favchannel *chan;

	chan = g_new (favchannel, 1);
	chan->name = g_strdup (channel);
	chan->key = g_strdup (key);
	chanlist = g_slist_append (chanlist, chan);

	return chanlist;
}

void
servlist_favchan_add (ircnet *net, char *channel)
{
	int pos;
	char *name;
	char *key;

	if (strchr (channel, ',') != NULL)
	{
		pos = (int) (strchr (channel, ',') - channel);
		name = g_strndup (channel, pos);
		key = g_strdup (channel + pos + 1);
	}
	else
	{
		name = g_strdup (channel);
		key = NULL;
	}

	net->favchanlist = servlist_favchan_listadd (net->favchanlist, name, key);

	g_free (name);
	g_free (key);
}

void
servlist_server_remove (ircnet *net, ircserver *serv)
{
	g_free (serv->hostname);
	g_free (serv);
	net->servlist = g_slist_remove (net->servlist, serv);
}

static void
servlist_server_remove_all (ircnet *net)
{
	ircserver *serv;

	while (net->servlist)
	{
		serv = net->servlist->data;
		servlist_server_remove (net, serv);
	}
}

void
servlist_command_free (commandentry *entry)
{
	g_free (entry->command);
	g_free (entry);
}

void
servlist_command_remove (ircnet *net, commandentry *entry)
{
	servlist_command_free (entry);
	net->commandlist = g_slist_remove (net->commandlist, entry);
}

void
servlist_favchan_free (favchannel *channel)
{
	g_free (channel->name);
	g_free (channel->key);
	g_free (channel);
}

void
servlist_favchan_remove (ircnet *net, favchannel *channel)
{
	servlist_favchan_free (channel);
	net->favchanlist = g_slist_remove (net->favchanlist, channel);
}

static void
free_and_clear (char *str)
{
	if (str)
	{
		char *orig = str;
		while (*str)
			*str++ = 0;
		g_free (orig);
	}
}

/* executed on exit: Clear any password strings */

void
servlist_cleanup (void)
{
	GSList *list;
	ircnet *net;

	for (list = network_list; list; list = list->next)
	{
		net = list->data;
		free_and_clear (net->pass);
	}
}

void
servlist_net_remove (ircnet *net)
{
	GSList *list;
	server *serv;

	servlist_server_remove_all (net);
	network_list = g_slist_remove (network_list, net);

	g_free (net->nick);
	g_free (net->nick2);
	g_free (net->user);
	g_free (net->real);
	free_and_clear (net->pass);
	if (net->favchanlist)
		g_slist_free_full (net->favchanlist, (GDestroyNotify) servlist_favchan_free);
	if (net->commandlist)
		g_slist_free_full (net->commandlist, (GDestroyNotify) servlist_command_free);
	g_free (net->encoding);
	g_free (net->name);
	g_free (net);

	/* for safety */
	list = serv_list;
	while (list)
	{
		serv = list->data;
		if (serv->network == net)
		{
			serv->network = NULL;
		}
		list = list->next;
	}
}

ircnet *
servlist_net_add (char *name, char *comment, int prepend)
{
	ircnet *net;

	net = g_new0 (ircnet, 1);
	net->name = g_strdup (name);
	net->flags = FLAG_CYCLE | FLAG_USE_GLOBAL | FLAG_USE_PROXY;
#ifdef USE_OPENSSL
	net->flags |= FLAG_USE_SSL;
#endif

	if (prepend)
		network_list = g_slist_prepend (network_list, net);
	else
		network_list = g_slist_append (network_list, net);

	return net;
}

static void
servlist_load_defaults (void)
{
	int i = 0, j = 0;
	ircnet *net = NULL;
	guint def_hash = g_str_hash ("Libera.Chat");

	while (1)
	{
		if (def[i].network)
		{
			net = servlist_net_add (def[i].network, def[i].host, FALSE);
			if (def[i].channel)
			{
				servlist_favchan_add (net, def[i].channel);
			}
			if (def[i].charset)
			{
				net->encoding = g_strdup (def[i].charset);
			}
			else
			{
				net->encoding = g_strdup (IRC_DEFAULT_CHARSET);
			}
			if (def[i].loginmode)
			{
				net->logintype = def[i].loginmode;
			}
			if (def[i].connectcmd)
			{
				servlist_command_add (net, def[i].connectcmd);
			}
			if (def[i].ssl)
			{
				net->flags |= FLAG_USE_SSL;
			}

			if (g_str_hash (def[i].network) == def_hash)
			{
				prefs.hex_gui_slist_select = j;
			}

			j++;
		}
		else
		{
			servlist_server_add (net, def[i].host);
			if (!def[i+1].host && !def[i+1].network)
			{
				break;
			}
		}
		i++;
	}
}

static int
servlist_load (void)
{
	FILE *fp;
	char buf[2048];
	int len;
	ircnet *net = NULL;

	/* simple migration we will keep for a short while */
	char *oldfile = g_build_filename (get_xdir (), "servlist_.conf", NULL);
	char *newfile = g_build_filename (get_xdir (), "servlist.conf", NULL);

	if (g_file_test (oldfile, G_FILE_TEST_EXISTS) && !g_file_test (newfile, G_FILE_TEST_EXISTS))
	{
		g_rename (oldfile, newfile);
	}

	g_free (oldfile);
	g_free (newfile);

	fp = hexchat_fopen_file ("servlist.conf", "r", 0);
	if (!fp)
		return FALSE;

	while (fgets (buf, sizeof (buf) - 2, fp))
	{
		len = strlen (buf);
		if (!len)
			continue;
		buf[len] = 0;
		buf[len-1] = 0;	/* remove the trailing \n */
		if (net)
		{
			switch (buf[0])
			{
			case 'I':
				net->nick = g_strdup (buf + 2);
				break;
			case 'i':
				net->nick2 = g_strdup (buf + 2);
				break;
			case 'U':
				net->user = g_strdup (buf + 2);
				break;
			case 'R':
				net->real = g_strdup (buf + 2);
				break;
			case 'P':
				net->pass = g_strdup (buf + 2);
				break;
			case 'L':
				net->logintype = atoi (buf + 2);
				break;
			case 'E':
				net->encoding = servlist_check_encoding (buf + 2) ? g_strdup (buf + 2) : g_strdup ("UTF-8");
				break;
			case 'F':
				net->flags = atoi (buf + 2);
				break;
			case 'S':	/* new server/hostname for this network */
				servlist_server_add (net, buf + 2);
				break;
			case 'C':
				servlist_command_add (net, buf + 2);
				break;
			case 'J':
				servlist_favchan_add (net, buf + 2);
				break;
			case 'D':
				net->selected = atoi (buf + 2);
				break;
			/* FIXME Migration code. In 2.9.5 the order was:
			 *
			 * P=serverpass, A=saslpass, B=nickservpass
			 *
			 * So if server password was unset, we can safely use SASL
			 * password for our new universal password, or if that's also
			 * unset, use NickServ password.
			 *
			 * Should be removed at some point.
			 */
			case 'A':
				if (!net->pass)
				{
					net->pass = g_strdup (buf + 2);
					if (!net->logintype)
					{
						net->logintype = LOGIN_SASL;
					}
				}
			case 'B':
				if (!net->pass)
				{
					net->pass = g_strdup (buf + 2);
					if (!net->logintype)
					{
						net->logintype = LOGIN_NICKSERV;
					}
				}
			}
		}
		if (buf[0] == 'N')
			net = servlist_net_add (buf + 2, /* comment */ NULL, FALSE);
	}
	fclose (fp);

	return TRUE;
}

void
servlist_init (void)
{
	if (!network_list)
		if (!servlist_load ())
			servlist_load_defaults ();
}

/* check if a charset is known by Iconv */
int
servlist_check_encoding (char *charset)
{
	GIConv gic;
	char *c;

	c = strchr (charset, ' ');
	if (c)
		c[0] = 0;

	gic = g_iconv_open (charset, "UTF-8");

	if (c)
		c[0] = ' ';

	if (gic != (GIConv)-1)
	{
		g_iconv_close (gic);
		return TRUE;
	}

	return FALSE;
}

int
servlist_save (void)
{
	FILE *fp;
	char *buf;
	ircnet *net;
	ircserver *serv;
	commandentry *cmd;
	favchannel *favchan;
	GSList *list;
	GSList *netlist;
	GSList *cmdlist;
	GSList *favlist;
#ifndef WIN32
	int first = FALSE;

	buf = g_build_filename (get_xdir (), "servlist.conf", NULL);
	if (g_access (buf, F_OK) != 0)
		first = TRUE;
#endif

	fp = hexchat_fopen_file ("servlist.conf", "w", 0);
	if (!fp)
	{
#ifndef WIN32
		g_free (buf);
#endif
		return FALSE;
	}

#ifndef WIN32
	if (first)
		g_chmod (buf, 0600);

	g_free (buf);
#endif
	fprintf (fp, "v=" PACKAGE_VERSION "\n\n");

	list = network_list;
	while (list)
	{
		net = list->data;

		fprintf (fp, "N=%s\n", net->name);
		if (net->nick)
			fprintf (fp, "I=%s\n", net->nick);
		if (net->nick2)
			fprintf (fp, "i=%s\n", net->nick2);
		if (net->user)
			fprintf (fp, "U=%s\n", net->user);
		if (net->real)
			fprintf (fp, "R=%s\n", net->real);
		if (net->pass)
			fprintf (fp, "P=%s\n", net->pass);
		if (net->logintype)
			fprintf (fp, "L=%d\n", net->logintype);
		if (net->encoding)
		{
			fprintf (fp, "E=%s\n", net->encoding);
			if (!servlist_check_encoding (net->encoding))
			{
				buf = g_strdup_printf (_("Warning: \"%s\" character set is unknown. No conversion will be applied for network %s."),
							 net->encoding, net->name);
				fe_message (buf, FE_MSG_WARN);
				g_free (buf);
			}
		}

		fprintf (fp, "F=%d\nD=%d\n", net->flags, net->selected);

		netlist = net->servlist;
		while (netlist)
		{
			serv = netlist->data;
			fprintf (fp, "S=%s\n", serv->hostname);
			netlist = netlist->next;
		}

		cmdlist = net->commandlist;
		while (cmdlist)
		{
			cmd = cmdlist->data;
			fprintf (fp, "C=%s\n", cmd->command);
			cmdlist = cmdlist->next;
		}

		favlist = net->favchanlist;
		while (favlist)
		{
			favchan = favlist->data;

			if (favchan->key)
			{
				fprintf (fp, "J=%s,%s\n", favchan->name, favchan->key);
			}
			else
			{
				fprintf (fp, "J=%s\n", favchan->name);
			}

			favlist = favlist->next;
		}

		if (fprintf (fp, "\n") < 1)
		{
			fclose (fp);
			return FALSE;
		}

		list = list->next;
	}

	fclose (fp);
	return TRUE;
}

static int
joinlist_find_chan (favchannel *curr_item, const char *channel)
{
	if (!g_ascii_strcasecmp (curr_item->name, channel))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

gboolean
joinlist_is_in_list (server *serv, char *channel)
{
	if (!serv->network || !((ircnet *)serv->network)->favchanlist)
	{
		return FALSE;
	}

	if (g_slist_find_custom (((ircnet *)serv->network)->favchanlist, channel, (GCompareFunc) joinlist_find_chan))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
