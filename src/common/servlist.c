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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "xchat.h"

#include "cfgfiles.h"
#include "fe.h"
#include "server.h"
#include "text.h"
#include "xchatc.h"

#include "servlist.h"


struct defaultserver
{
	char *network;
	char *host;
	char *channel;
};

static const struct defaultserver def[] =
{
	{"2600net",	0},
	{0,			"irc.2600.net"},
	{0,			"ptolomea.2600.net"},
	{0,			"collective.2600.net"},

	{"AccessIRC",	0},
	{0,			"irc.accessirc.net"},
	{0,			"eu.accessirc.net"},

	{"AfterNET",	0},
	{0,			"irc.afternet.org"},
	{0,			"us.afternet.org"},
	{0,			"eu.afternet.org"},

	{"Aitvaras",	0},
#ifdef USE_IPV6
#ifdef USE_OPENSSL
	{0,			"irc6.ktu.lt/+7668"},
#endif
	{0,			"irc6.ktu.lt/7666"},
#endif
#ifdef USE_OPENSSL
	{0,			"irc.data.lt/+6668"},
#endif
	{0,			"irc.data.lt"},
	{0,			"irc.ktu.lt"},
	{0,			"irc.le.lt"},
	{0,			"irc.omnitel.net"},
	{0,			"irc.takas.lt"},
	{0,			"irc.5ci.net"},
	{0,			"irc.interneka.lt"},
	{0,			"irc.elekta.lt"},
	{0,			"irc.if.lt"},

	{"AmigaNet",	0},
	{0,			"irc.amiganet.org"},
	{0,			"us.amiganet.org"},
	{0,			"uk.amiganet.org"},
	{0,			"no.amiganet.org"},
	{0,			"au.amiganet.org"},

	{"ARCNet",	0},
	{0,			"se1.arcnet.vapor.com"},
	{0,			"us1.arcnet.vapor.com"},
	{0,			"us2.arcnet.vapor.com"},
	{0,			"us3.arcnet.vapor.com"},
	{0,			"ca1.arcnet.vapor.com"},
	{0,			"de1.arcnet.vapor.com"},
	{0,			"de3.arcnet.vapor.com"},
	{0,			"ch1.arcnet.vapor.com"},
	{0,			"be1.arcnet.vapor.com"},
	{0,			"nl3.arcnet.vapor.com"},
	{0,			"uk1.arcnet.vapor.com"},
	{0,			"uk2.arcnet.vapor.com"},
	{0,			"uk3.arcnet.vapor.com"},
	{0,			"fr1.arcnet.vapor.com"},

	{"AstroLink",	0},
	{0,			"irc.astrolink.org"},

	{"AustNet",	0},
	{0,			"au.austnet.org"},
	{0,			"us.austnet.org"},
	{0,			"ca.austnet.org"},

	{"AxeNet",	0},
	{0,			"irc.axenet.org"},
	{0,			"angel.axenet.org"},
	{0,			"energy.axenet.org"},
	{0,			"python.axenet.org"},

	{"AzzurraNet",	0},
	{0,			"irc.azzurra.org"},
	{0,			"crypto.azzurra.org"},

	{"Buzzpot", 0},
	{0,			"irc.chatspike.net"},

	{"ChatJunkies",	0, "#xchat"},
	{0,			"irc.chatjunkies.org"},
	{0,			"nl.chatjunkies.org"},

	{"ChatNet",	0},
	{0,			"US.ChatNet.Org"},
	{0,			"EU.ChatNet.Org"},

	{"ChatSociety", 0},
	{0,			"us.chatsociety.net"},
	{0,			"eu.chatsociety.net"},

	{"CoolChat",	0},
	{0,			"irc.coolchat.net"},
	{0,			"unix.coolchat.net"},
	{0,			"toronto.coolchat.net"},

	{"DALnet",	0},
	{0,			"irc.dal.net"},
	{0,			"irc.eu.dal.net"},

	{"Dark-Tou-Net",	0},
	{0,			"irc.d-t-net.de"},
	{0,			"bw.d-t-net.de"},
	{0,			"nc.d-t-net.de"},
	{0,			"wakka.d-t-net.de"},

	{"DeepIRC", 0},
	{0,			"irc.deepirc.net"},

	{"EFnet",	0},
	{0,			"irc.Prison.NET"},
	{0,			"irc.Qeast.net"},
	{0,			"irc.efnet.pl"},
	{0,			"efnet.demon.co.uk"},
	{0,			"irc.lagged.org"},
	{0,			"irc.lightning.net"},
	{0,			"irc.mindspring.com"},
	{0,			"irc.rt.ru"},
	{0,			"irc.easynews.com"},
	{0,			"irc.servercentral.net"},

	{"EUIrc",	0},
	{0,			"irc.euirc.net"},
	{0,			"irc.ham.de.euirc.net"},
	{0,			"irc.ber.de.euirc.net"},
	{0,			"irc.ffm.de.euirc.net"},
	{0,			"irc.bre.de.euirc.net"},
	{0,			"irc.hes.de.euirc.net"},
	{0,			"irc.vie.at.euirc.net"},
	{0,			"irc.inn.at.euirc.net"},
	{0,			"irc.bas.ch.euirc.net"},

	{"EuropNet", 0},
	{0,			"irc.europnet.org"},

	{"EU-IRC",	0},
	{0,			"irc.eu-irc.net"},

	{"FDFNet",	0},
	{0,			"irc.fdfnet.net"},
	{0,			"irc.eu.fdfnet.net"},

	{"FEFNet",	0},
	{0,			"irc.fef.net"},
	{0,			"irc.villagenet.com"},
	{0,			"irc.ggn.net"},
	{0,			"irc.vendetta.com"},

	{"FreeNode",	0},
	{0,				"irc.freenode.net"},

	{"Freeworld",	0},
	{0,			"kabel.freeworld.nu"},
	{0,			"irc.freeworld.nu"},

	{"GalaxyNet",	0},
	{0,			"irc.galaxynet.org"},
	{0,			"sprynet.us.galaxynet.org"},
	{0,			"atlanta.ga.us.galaxynet.org"},

	{"GamesNET",	0},
	{0,				"irc.gamesnet.net"},
	{0,				"irc.us.gamesnet.net"},
	{0,				"east.us.gamesnet.net"},
	{0,				"west.us.gamesnet.net"},
	{0,				"irc.ca.gamesnet.net"},
	{0,				"irc.eu.gamesnet.net"},

	{"German-Elite",	0},
	{0,			"dominion.german-elite.net"},
	{0,			"komatu.german-elite.net"},
	{0,			"liberty.german-elite.net"},

	{"GimpNet",		0},
	{0,			"irc.gimp.org"},
	{0,			"irc.au.gimp.org"},
	{0,			"irc.us.gimp.org"},

	{"HabberNet",	0},
	{0,			"irc.habber.net"},

	{"Hashmark",	0},
	{0,			"irc.hashmark.net"},

	{"Infinity-IRC",	0},
	{0,			"Atlanta.GA.US.Infinity-IRC.Org"},
	{0,			"Babylon.NY.US.Infinity-IRC.Org"},
	{0,			"Sunshine.Ca.US.Infinity-IRC.Org"},
	{0,			"IRC.Infinity-IRC.Org"},

	{"insiderZ.DE",	0},
	{0,			"irc.insiderz.de/6667"},
	{0,			"irc.insiderz.de/6666"},

	{"IrcLink",	0},
	{0,			"irc.irclink.net"},
	{0,			"Alesund.no.eu.irclink.net"},
	{0,			"Oslo.no.eu.irclink.net"},
	{0,			"frogn.no.eu.irclink.net"},
	{0,			"tonsberg.no.eu.irclink.net"},

	{"IRCNet",		0},
	{0,				"irc.stealth.net/5550"},
	{0,				"ircnet.demon.co.uk"},
	{0,				"ircnet.hinet.hr"},
	{0,				"irc.datacomm.ch"},
	{0,				"ircnet.kaptech.fr"},
	{0,				"irc.flashnet.it"},
	{0,				"irc.cwitaly.it"},
	{0,				"ircnet.easynet.co.uk"},
	{0,				"random.ircd.de"},
	{0,				"ircnet.netvision.net.il"},
	{0,				"irc.seed.net.tw"},
	{0,				"irc.cs.hut.fi"},

	{"IRCSoulZ",	0},
	{0,			"irc.ircsoulz.net"},

	{"Irctoo.net",	0},
	{0,			"irc.canadian.net"},
	{0,			"irc.irctoo.net"},

	{"Librenet",	0},
	{0,			"irc.librenet.net"},
	{0,			"ielf.fr.librenet.net"},

	{"LinkNet",	0},
	{0,			"irc.link-net.org"},
	{0,			"irc.no.link-net.org"},
	{0,			"irc.gamesden.net.au"},
	{0,			"irc.bahnhof.se"},
	{0,			"irc.kinexuseurope.co.uk"},
	{0,			"irc.gamiix.com"},

	{"MagicStar",	0},
	{0,			"irc.magicstar.net"},

	{"Majistic",	0},
	{0,			"irc.majistic.net"},

	{"NeverNET",	0},
	{0,			"irc.nevernet.net"},
	{0,			"imagine.nevernet.net"},
	{0,			"dimension.nevernet.net"},
	{0,			"universe.nevernet.net"},
	{0,			"wayland.nevernet.net"},
	{0,			"forte.nevernet.net"},

	{"NixHelpNet",	0},
	{0,			"irc.nixhelp.org"},
	{0,			"us.nixhelp.org"},
	{0,			"uk.nixhelp.org"},
	{0,			"uk2.nixhelp.org"},
	{0,			"uk3.nixhelp.org"},
	{0,			"nl.nixhelp.org"},
	{0,			"ca.ld.nixhelp.org"},
	{0,			"us.co.nixhelp.org"},
	{0,			"us.ca.nixhelp.org"},
	{0,			"us.pa.nixhelp.org"},

	{"NullusNet",	0},
	{0,			"irc.nullus.net"},

	{"OFTC",	0},
	{0,			"irc.oftc.net"},

	{"OnlyIRC",		0},
	{0,			"irc.onlyirc.net"},

	{"OtherNet",	0},
	{0,			"irc.othernet.org"},

	{"OzNet",	0},
	{0,			"sydney.oz.org"},
	{0,			"melbourne.oz.org"},

	{"Progameplayer",	0},
	{0,			"irc.progameplayer.com"},
	{0,			"melancholia.oh.us.progameplayer.com"},
	{0,			"deimos.oh.us.progameplayer.com"},
	{0,			"paradigm.oh.us.progameplayer.com"},

	{"PTlink",	0},
	{0,			"irc.PTlink.net"},
	{0,			"aaia.PTlink.net"},

	{"PTNet, ISP's",	0},
	{0,			"irc.PTNet.org"},
	{0,			"rccn.PTnet.org"},
	{0,			"EUnet.PTnet.org"},
	{0,			"madinfo.PTnet.org"},
	{0,			"netc2.PTnet.org"},
	{0,			"netc1.PTnet.org"},
	{0,			"telepac1.ptnet.org"},
	{0,			"esoterica.PTnet.org"},
	{0,			"ip-hub.ptnet.org"},
	{0,			"telepac1.ptnet.org"},
	{0,			"nortenet.PTnet.org"},

	{"PTNet, UNI",	0},
	{0,			"irc.PTNet.org"},
	{0,			"rccn.PTnet.org"},
	{0,			"uevora.PTnet.org"},
	{0,			"umoderna.PTnet.org"},
	{0,			"ist.PTnet.org"},
	{0,			"aaum.PTnet.org"},
	{0,			"uc.PTnet.org"},
	{0,			"ualg.ptnet.org"},
	{0,			"madinfo.PTnet.org"},
	{0,			"isep.PTnet.org"},
	{0,			"ua.PTnet.org"},
	{0,			"ipg.PTnet.org"},
	{0,			"isec.PTnet.org"},
	{0,			"utad.PTnet.org"},
	{0,			"iscte.PTnet.org"},
	{0,			"ubi.PTnet.org"},

	{"QChat.net",	0},
	{0,			"irc.qchat.net"},

	{"QuakeNet",	0},
	{0,			"irc.quakenet.org"},
	{0,			"irc.se.quakenet.org"},
	{0,			"irc.dk.quakenet.org"},
	{0,			"irc.no.quakenet.org"},
	{0,			"irc.fi.quakenet.org"},
	{0,			"irc.be.quakenet.org"},
	{0,			"irc.uk.quakenet.org"},
	{0,			"irc.de.quakenet.org"},
	{0,			"irc.it.quakenet.org"},

	{"RealmNET",	0},
	{0,			"irc.realmnet.com"},
	{0,			"irc.eu.realmnet.com"},
	{0,			"irc.us.realmnet.com"},

	{"RebelChat",	0},
	{0,			"irc.rebelchat.org"},

	{"Recycled-IRC",  0},
	{0,			"irc.recycled-irc.org"},
	{0,			"vermin.recycled-irc.org"},
	{0,			"waste.recycled-irc.org"},
	{0,			"lumber.recycled-irc.org"},
	{0,			"trash.recycled-irc.org"},
	{0,			"unwashed.recycled-irc.org"},
	{0,			"garbage.recycled-irc.org"},
	{0,			"dustbin.recycled-irc.org"},

	{"RedeBrasilIRC",	0},
	{0,			"irc.redebrasilirc.org"},

	{"RizeNET", 0},
	{0,			"irc.rizenet.org"},
	{0,			"omega.rizenet.org"},
	{0,			"evelance.rizenet.org"},
	{0,			"lisa.rizenet.org"},
	{0,			"scott.rizenet.org"},

	{"SceneNet",	0},
	{0,			"irc.scene.org"},
	{0,			"irc.eu.scene.org"},
	{0,			"irc.us.scene.org"},

	{"SlashNET",	0},
	{0,			"irc.slashnet.org"},
	{0,			"area51.slashnet.org"},
	{0,			"moo.slashnet.org"},
	{0,			"radon.slashnet.org"},

	{"Sohbet.Net", 0},
	{0,			"irc.sohbet.net"},

	{"SorceryNet",	0},
	{0,			"irc.sorcery.net/9000"},
	{0,			"irc.us.sorcery.net/9000"},
	{0,			"irc.eu.sorcery.net/9000"},

	{"Spidernet",	0},
	{0,			"us.spidernet.org"},
	{0,			"eu.spidernet.org"},
	{0,			"irc.spidernet.org"},

	{"StarChat", 0},
	{0,			"irc.starchat.net"},
	{0,			"gainesville.starchat.net"},
	{0,			"freebsd.starchat.net"},
	{0,			"sunset.starchat.net"},
	{0,			"revenge.starchat.net"},
	{0,			"tahoma.starchat.net"},
	{0,			"neo.starchat.net"},

	{"SubCultNet",	0},
	{0,			"irc.subcult.ch"},
	{0,			"irc.phuncrew.ch"},
	{0,			"irc.mgz.ch"},
	{0,			"irc.nazgul.ch/6666"},

	{"TNI3",			0},
	{0,			"irc.tni3.com"},

	{"TopGamers",	0},
	{0,			"irc.topgamers.net"},

	{"UnderNet",	0},
	{0,			"us.undernet.org"},
	{0,			"eu.undernet.org"},

	{"UniBG",		0},
	{0,			"irc.lirex.com"},
	{0,			"irc.naturella.com"},
	{0,			"irc.spnet.net"},
	{0,			"irc.techno-link.com"},
	{0,			"irc.telecoms.bg"},
	{0,			"irc.tu-varna.edu"},

	{"Unsecurity",	0},
	{0,			"irc.unsecurity.org"},

	{"Worldnet",		0},
	{0,			"irc.worldnet.net"},
	{0,			"irc.fr.worldnet.net"},

	{"Xentonix.net",	0},
	{0,			"irc.ffm.de.eu.xentonix.net"},
	{0,			"irc.brs.de.eu.xentonix.net"},
	{0,			"irc.stg.ch.eu.xentonix.net"},
	{0,			"irc.hou.tx.us.xentonix.net"},
	{0,			"irc.kar.de.eu.xentonix.net"},
	{0,			"irc.vie.at.eu.xentonix.net"},

	{"XWorld",	0},
	{0,			"Buffalo.NY.US.XWorld.org"},
	{0,			"Minneapolis.MN.US.Xworld.Org"},
	{0,			"Rochester.NY.US.XWorld.org"},
	{0,			"Bayern.DE.EU.XWorld.Org"},
	{0,			"Chicago.IL.US.XWorld.Org"},

	{0,0}
};

GSList *network_list = 0;


void
servlist_connect (session *sess, ircnet *net)
{
	ircserver *ircserv;
	GSList *list;
	char *port;
	char *space;
	server *serv;

	if (!sess)
		sess = new_ircwindow (NULL, NULL, SESS_SERVER, 0);

	serv = sess->server;

	/* connect to the currently selected Server-row */
	list = g_slist_nth (net->servlist, net->selected);
	if (!list)
		list = net->servlist;
	ircserv = list->data;

	/* incase a protocol switch is added to the servlist gui */
	server_fill_her_up (sess->server);

	if (net->autojoin && !sess->server->recondelay_tag)
		safe_strcpy (sess->willjoinchannel, net->autojoin,
						 sizeof (sess->willjoinchannel));
	if (net->pass)
		safe_strcpy (serv->password, net->pass, sizeof (serv->password));

	if (net->flags & FLAG_USE_GLOBAL)
	{
		strcpy (serv->nick, prefs.nick1);
	} else
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
		serv->connect (serv, ircserv->hostname, 6667, FALSE);

	if (serv->encoding)
	{
		free (serv->encoding);
		serv->encoding = NULL;
	}
	if (net->encoding)
	{
		serv->encoding = strdup (net->encoding);
		space = strchr (serv->encoding, ' ');
		if (space)
			space[0] = 0;
	}
}

int
servlist_connect_by_netname (session *sess, char *network)
{
	ircnet *net;
	GSList *list = network_list;

	while (list)
	{
		net = list->data;

		if (strcasecmp (net->name, network) == 0)
		{
			servlist_connect (sess, net);
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
	int ret = 0;

	while (list)
	{
		net = list->data;

		if (net->flags & FLAG_AUTO_CONNECT)
			ret = 1;

		list = list->next;
	}

	return ret;
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
			servlist_connect (sess, net);
			ret = 1;
		}

		list = list->next;
	}

	return ret;
}

static gint
servlist_cycle_cb (server *serv)
{
	PrintTextf (serv->server_session,
		_("Cycling to next server in %s...\n"), ((ircnet *)serv->network)->name);
	servlist_connect (serv->server_session, serv->network);

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

			del = prefs.recon_delay * 1000;
			if (del < 1000)
				del = 500;				  /* so it doesn't block the gui */

			if (del)
				serv->recondelay_tag = fe_timeout_add (del, servlist_cycle_cb, serv);
			else
				servlist_connect (serv->server_session, net);

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
				*pos = i;
			return serv;
		}
		i++;
		list = list->next;
	}

	return NULL;
}

ircnet *
servlist_net_find (char *name, int *pos)
{
	GSList *list = network_list;
	ircnet *net;
	int i = 0;

	while (list)
	{
		net = list->data;
		if (strcmp (net->name, name) == 0)
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

	serv = malloc (sizeof (ircserver));
	memset (serv, 0, sizeof (ircserver));
	serv->hostname = strdup (name);

	net->servlist = g_slist_append (net->servlist, serv);

	return serv;
}

void
servlist_server_remove (ircnet *net, ircserver *serv)
{
	free (serv->hostname);
	free (serv);
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
servlist_net_remove (ircnet *net)
{
	GSList *list;
	server *serv;

	servlist_server_remove_all (net);
	network_list = g_slist_remove (network_list, net);

	if (net->nick)
		free (net->nick);
	if (net->user)
		free (net->user);
	if (net->real)
		free (net->real);
	if (net->pass)
		free (net->pass);
	if (net->autojoin)
		free (net->autojoin);
	if (net->command)
		free (net->command);
	if (net->nickserv)
		free (net->nickserv);
	if (net->comment)
		free (net->comment);
	if (net->encoding)
		free (net->encoding);
	free (net->name);
	free (net);

	/* for safety */
	list = serv_list;
	while (list)
	{
		serv = list->data;
		if (serv->network == net)
			serv->network = NULL;
		list = list->next;
	}
}

ircnet *
servlist_net_add (char *name, char *comment, int prepend)
{
	ircnet *net;

	net = malloc (sizeof (ircnet));
	memset (net, 0, sizeof (ircnet));
	net->name = strdup (name);
/*	net->comment = strdup (comment);*/
	net->flags = FLAG_CYCLE | FLAG_USE_GLOBAL | FLAG_USE_PROXY;

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

	while (1)
	{
		if (def[i].network)
		{
			net = servlist_net_add (def[i].network, def[i].host, FALSE);
#ifdef WIN32
			/* Windows gets UTF-8 for new users. Unix gets "System Default",
				which is often UTF-8 anyway! */
			net->encoding = strdup ("UTF-8");
#endif
			if (def[i].channel)
				net->autojoin = strdup (def[i].channel);
			if (!strcmp (def[i].network, "ChatJunkies"))
				prefs.slist_select = j;
			j++;
		} else
		{
			servlist_server_add (net, def[i].host);
			if (!def[i+1].host && !def[i+1].network)
				break;
		}
		i++;
	}
}

static int
servlist_load (void)
{
	FILE *fp;
	char buf[258];
	int len;
	char *tmp;
	ircnet *net = NULL;

	snprintf (buf, sizeof (buf), "%s/servlist_.conf", get_xdir_fs ());
	fp = fopen (buf, "r");
	if (!fp)
		return FALSE;

	while (fgets (buf, sizeof (buf) - 2, fp))
	{
		len = strlen (buf);
		buf[len] = 0;
		buf[len-1] = 0;	/* remove the trailing \n */
		if (net)
		{
			switch (buf[0])
			{
			case 'I':
				net->nick = strdup (buf + 2);
				break;
			case 'U':
				net->user = strdup (buf + 2);
				break;
			case 'R':
				net->real = strdup (buf + 2);
				break;
			case 'P':
				net->pass = strdup (buf + 2);
				break;
			case 'J':
				net->autojoin = strdup (buf + 2);
				break;
			case 'C':
				if (net->command)
				{
					/* concat extra commands with a \n separator */
					tmp = net->command;
					net->command = malloc (strlen (tmp) + strlen (buf + 2) + 2);
					strcpy (net->command, tmp);
					strcat (net->command, "\n");
					strcat (net->command, buf + 2);
					free (tmp);
				} else
					net->command = strdup (buf + 2);
				break;
			case 'F':
				net->flags = atoi (buf + 2);
				break;
			case 'D':
				net->selected = atoi (buf + 2);
				break;
			case 'E':
				net->encoding = strdup (buf + 2);
				break;
			case 'S':	/* new server/hostname for this network */
				servlist_server_add (net, buf + 2);
				break;
			case 'B':
				net->nickserv = strdup (buf + 2);
				break;
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

static int
servlist_write_ccmd (char *str, void *fp)
{
	return fprintf (fp, "C=%s\n", (str[0] == '/') ? str + 1 : str);
}

/* separates a string according to a 'sep' char, then calls the callback
   function for each token found */

int
token_foreach (char *str, char sep,
					int (*callback) (char *str, void *ud), void *ud)
{
	char t, *start = str;

	while (1)
	{
		if (*str == sep || *str == 0)
		{
			t = *str;
			*str = 0;
			if (callback (start, ud) < 1)
			{
				*str = t;
				return FALSE;
			}
			*str = t;

			if (*str == 0)
				break;
			str++;
			start = str;

		} else
		{
			str += g_utf8_skip[*((unsigned char *)str)];
		}
	}

	return TRUE;
}

int
servlist_save (void)
{
	FILE *fp;
	char buf[256];
	ircnet *net;
	ircserver *serv;
	GSList *list;
	GSList *hlist;
	int first = FALSE;

	snprintf (buf, sizeof (buf), "%s/servlist_.conf", get_xdir_fs ());
	if (access (buf, F_OK) != 0)
		first = TRUE;
	fp = fopen (buf, "w");
	if (!fp)
		return FALSE;

#ifndef WIN32
	if (first)
		chmod (buf, 0600);
#endif
	fprintf (fp, "v="VERSION"\n\n");

	list = network_list;
	while (list)
	{
		net = list->data;

		fprintf (fp, "N=%s\n", net->name);
		if (net->nick)
			fprintf (fp, "I=%s\n", net->nick);
		if (net->user)
			fprintf (fp, "U=%s\n", net->user);
		if (net->real)
			fprintf (fp, "R=%s\n", net->real);
		if (net->pass)
			fprintf (fp, "P=%s\n", net->pass);
		if (net->autojoin)
			fprintf (fp, "J=%s\n", net->autojoin);
		if (net->nickserv)
			fprintf (fp, "B=%s\n", net->nickserv);
		if (net->encoding && strcasecmp (net->encoding, "System") &&
			 strcasecmp (net->encoding, "System default"))
		{
			fprintf (fp, "E=%s\n", net->encoding);
			if (!servlist_check_encoding (net->encoding))
			{
				snprintf (buf, sizeof (buf), _("Warning: \"%s\" character set is unknown. No conversion will be applied for network %s."),
							 net->encoding, net->name);
				fe_message (buf, FALSE);
			}
		}

		if (net->command)
			token_foreach (net->command, '\n', servlist_write_ccmd, fp);

		fprintf (fp, "F=%d\nD=%d\n", net->flags, net->selected);

		hlist = net->servlist;
		while (hlist)
		{
			serv = hlist->data;
			fprintf (fp, "S=%s\n", serv->hostname);
			hlist = hlist->next;
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
