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
	{0,			"irc.dc2600.com"},
	{0,			"irc.nyc2600.com"},

	{"AbleNET",	0},
	{0,			"california.ablenet.org"},
	{0,			"amazon.ablenet.org"},
	{0,			"agora.ablenet.org"},
	{0,			"extreme.ablenet.org"},
	{0,			"irc.ablenet.org"},

	{"AfterNET",	0},
	{0,			"irc.afternet.org"},
	{0,			"ic5.eu.afternet.org"},
	{0,			"baltimore.md.us.afternet.org"},
	{0,			"boston.afternet.org"},

	{"AmigaNet",	0},
	{0,			"linux.us.amiganet.org"},
	{0,			"whiterose.us.amiganet.org"},
	{0,			"thule.no.amiganet.org"},
	{0,			"dynarc.se.amiganet.org"},
	{0,			"fullcomp.au.amiganet.org"},
	{0,			"spod.uk.amiganet.org"},

	{"AnyNet",	0},
	{0,			"irc.anynet.org"},

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
	{0,			"us.austnet.org"},
	{0,			"ca.austnet.org"},
	{0,			"au.austnet.org"},

	{"AxeNet",	0},
	{0,			"irc.axenet.org"},
	{0,			"angel.axenet.org"},
	{0,			"energy.axenet.org"},
	{0,			"python.axenet.org"},

	{"AzzurraNet",	0},
	{0,			"irc.azzurra.org"},
	{0,			"crypto.azzurra.org"},

	{"ChatJunkies",	0, "#Linux"},
	{0,			"irc.chatjunkies.org"},
	{0,			"nl.chatjunkies.org"},

	{"ChatNet",	0},
	{0,			"US.ChatNet.Org"},
	{0,			"EU.ChatNet.Org"},

	{"CoolChat",	0},
	{0,			"irc.coolchat.net"},
	{0,			"unix.coolchat.net"},
	{0,			"toronto.coolchat.net"},

	{"CyberArmy",	0},
	{0,			"irc.cyberarmy.com"},

	{"DALnet",	0},
	{0,			"irc.dal.net"},
	{0,			"irc.eu.dal.net"},

	{"Dark-Tou-Net",	0},
	{0,			"irc.d-t-net.de"},
	{0,			"bw.d-t-net.de"},
	{0,			"nc.d-t-net.de"},
	{0,			"wakka.d-t-net.de"},

	{"DwarfStarNet",	0},
	{0,			"IRC.dwarfstar.net"},
	{0,			"US.dwarfstar.net"},
	{0,			"EU.dwarfstar.net"},
	{0,			"AU.dwarfstar.net"},

	{"EFnet",	0},
	{0,			"irc.Prison.NET"},
	{0,			"irc.Qeast.net"},
	{0,			"irc.efnet.pl"},
	{0,			"irc.flamed.net"},
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

	{"Evolnet",	0},
	{0,			"irc.evolnet.org"},
	{0,			"blender.evolnet.org"},

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
	{0,				"irc.eu.freenode.net"},
	{0,				"irc.au.freenode.net"},
	{0,				"irc.us.freenode.net"},

	{"Freeworld",	0},
	{0,			"kabel.freeworld.nu"},
	{0,			"irc.freeworld.nu"},

	{"GalaxyNet",	0},
	{0,			"irc.galaxynet.org"},
	{0,			"sprynet.us.galaxynet.org"},
	{0,			"atlanta.ga.us.galaxynet.org"},

	{"Gamma Force",	0},
	{0,			"irc.gammaforce.org"},
	{0,			"sphinx.or.us.gammaforce.org"},

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

	{"IRCDZone",		0},
	{0,			"irc.ircdzone.net"},

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

	{"KewlNet",	0},
	{0,			"random.irc.kewl.org"},
	{0,			"la.defense.fr.eu.kewl.org"},
	{0,			"nanterre.fr.eu.kewl.org"},

	{"KrushNet",	0},
	{0,			"Jeffersonville.IN.US.KrushNet.Org"},
	{0,			"Auckland.NZ.KrushNet.Org"},
	{0,			"Hastings.NZ.KrushNet.Org"},
	{0,			"Seattle-R.WA.US.KrushNet.Org"},
	{0,			"Minneapolis.MN.US.KrushNet.Org"},
	{0,			"Cullowhee.NC.US.KrushNet.Org"},
	{0,			"Asheville-R.NC.US.KrushNet.Org"},
	{0,			"San-Antonio.TX.US.KrushNet.Org"},

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
	{0,			"irc.spot.org"},
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

	{"Spidernet",	0},
	{0,			"us.spidernet.org"},
	{0,			"eu.spidernet.org"},
	{0,			"irc.spidernet.org"},

	{"StarChat",	0},
	{0,			"irc.starchat.net"},
	{0,			"galatea.starchat.net"},
	{0,			"stargate.starchat.net"},
	{0,			"powerzone.starchat.net"},
	{0,			"utopia.starchat.net"},
	{0,			"cairns.starchat.net"},

	{"SubCultNet",	0},
	{0,			"irc.subcult.ch"},
	{0,			"irc.phuncrew.ch"},
	{0,			"irc.mgz.ch"},

	{"TNI3",			0},
	{0,			"irc.tni3.com"},

	{"TopGamers",	0},
	{0,			"irc.topgamers.net"},

	{"UnderNet",	0},
	{0,			"us.undernet.org"},
	{0,			"eu.undernet.org"},

	{"Unsecurity",	0},
	{0,			"irc.unsecurity.org"},

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
		sess = new_ircwindow (NULL, NULL, SESS_SERVER);

	serv = sess->server;

	/* connect to the currently selected Server-row */
	list = g_slist_nth (net->servlist, net->selected);
	if (!list)
		list = net->servlist;
	ircserv = list->data;

	/* incase a protocol switch is added to the servlist gui */
	server_fill_her_up (sess->server);

	if (net->autojoin)
		safe_strcpy (sess->willjoinchannel, net->autojoin,
						 sizeof (sess->willjoinchannel));
	if (net->pass)
		safe_strcpy (serv->password, net->pass, sizeof (serv->password));

	if (serv->username)
	{
		free (serv->username);
		serv->username = NULL;
	}
	if (serv->realname)
	{
		free (serv->realname);
		serv->realname = NULL;
	}

	if (net->flags & FLAG_USE_GLOBAL)
	{
		strcpy (serv->nick, prefs.nick1);
	} else
	{
		if (net->nick)
			strcpy (serv->nick, net->nick);
		if (net->user)
			serv->username = strdup (net->user);
		if (net->real)
			serv->realname = strdup (net->real);
	}

	serv->dont_use_proxy = (net->flags & FLAG_USE_PROXY) ? FALSE : TRUE;

#ifdef USE_OPENSSL
	serv->use_ssl = (net->flags & FLAG_USE_SSL) ? TRUE : FALSE;
	serv->accept_invalid_cert =
		(net->flags & FLAG_ALLOW_INVALID) ? TRUE : FALSE;
#endif

	if (net->flags & FLAG_CYCLE)
		/* needed later to do cycling */
		serv->network = net;
	else
		serv->network = NULL;

	serv->networkname = strdup (net->name);

	port = strrchr (ircserv->hostname, '/');
	if (port)
	{
		*port = 0;
		serv->connect (serv, ircserv->hostname, atoi (port + 1), FALSE);
		*port = '/';
	} else
		serv->connect (serv, ircserv->hostname, 6667, FALSE);

	if (net->command)
	{
		if (serv->eom_cmd)
			free (serv->eom_cmd);
		serv->eom_cmd = strdup (net->command[0] == '/' ? net->command+1 : net->command);
	}

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
		"Cycling to next server in %s...\n", ((ircnet *)serv->network)->name);
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
			net->selected++;
			if (net->selected >= max)
				net->selected = 0;

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
servlist_net_add (char *name, char *comment)
{
	ircnet *net;

	net = malloc (sizeof (ircnet));
	memset (net, 0, sizeof (ircnet));
	net->name = strdup (name);
/*	net->comment = strdup (comment);*/
	net->flags = FLAG_CYCLE | FLAG_USE_GLOBAL | FLAG_USE_PROXY;

	network_list = g_slist_append (network_list, net);

	return net;
}

static void
servlist_load_defaults (void)
{
	int i = 0;
	ircnet *net = NULL;

	while (1)
	{
		if (def[i].network)
		{
			net = servlist_net_add (def[i].network, def[i].host);
			if (def[i].channel)
				net->autojoin = strdup (def[i].channel);
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
	char buf[256];
	int len;
	ircnet *net = NULL;

	snprintf (buf, sizeof (buf), "%s/servlist_.conf", get_xdir ());
	fp = fopen (buf, "r");
	if (!fp)
		return FALSE;

	while (fgets (buf, sizeof (buf) - 1, fp))
	{
		len = strlen (buf);
		buf[len-1] = 0;	/* remove the trailing \n */
		switch (buf[0])
		{
			case 'N':
				net = servlist_net_add (buf + 2, /* comment */ "");
				break;
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
		}
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

void
servlist_save (void)
{
	FILE *fp;
	char buf[256];
	ircnet *net;
	ircserver *serv;
	GSList *list;
	GSList *hlist;
	int first = FALSE;

	snprintf (buf, sizeof (buf), "%s/servlist_.conf", get_xdir ());
	if (access (buf, F_OK) != 0)
		first = TRUE;
	fp = fopen (buf, "w");
	if (!fp)
		return;

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
			fprintf (fp, "C=%s\n", net->command[0] == '/' ? net->command+1 : net->command);
		fprintf (fp, "F=%d\nD=%d\n", net->flags, net->selected);

		hlist = net->servlist;
		while (hlist)
		{
			serv = hlist->data;
			fprintf (fp, "S=%s\n", serv->hostname);
			hlist = hlist->next;
		}
		fprintf (fp, "\n");

		list = list->next;
	}

	fclose (fp);
}
