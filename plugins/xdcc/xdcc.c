/* xchat 2.0 plugin: simple xdcc server example */

#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "xchat-plugin.h"

static xchat_plugin *ph;	/* plugin handle */

static int xdcc_on = 1;
static int xdcc_slots = 3;
static GSList *file_list = 0;

typedef struct fileoffer
{
	char *file;
	char *fullpath;
	char *desc;
	int downloads;
} fileoffer;


/* find the number of open dccs */

static int num_open_dccs(void)
{
	xchat_list *list;
	int num = 0;

	list = xchat_list_get(ph, "dcc");
	if(!list)
		return 0;

	while(xchat_list_next(ph, list))
	{
		/* check only ACTIVE dccs */
		if(xchat_list_int(ph, list, "status") == 1)
		{
			/* check only SEND dccs */
			if(xchat_list_int(ph, list, "type") == 0)
				num++;
		}
	}

	xchat_list_free(ph, list);

	return num;
}

static void xdcc_get(char *nick, char *host, char *arg)
{
	int num;
	GSList *list;
	fileoffer *offer;

	if(arg[0] == '#')
		arg++;

	num = atoi(arg);
	list = g_slist_nth(file_list, num - 1);
	if(!list)
	{
		xchat_commandf(ph, "quote NOTICE %s :No such file number #%d!", nick, num);
		return;
	}

	if(num_open_dccs() >= xdcc_slots)
	{
		xchat_commandf(ph, "quote NOTICE %s :All slots full. Try again later.", nick);
		return;
	}

	offer = (fileoffer *) list->data;
	offer->downloads++;
	xchat_commandf(ph, "quote NOTICE %s :Sending offer #%d %s", nick, num, offer->file);
	xchat_commandf(ph, "dcc send %s %s", nick, offer->fullpath);
}

static void xdcc_del(char *name)
{
	GSList *list;
	fileoffer *offer;

	list = file_list;
	while(list)
	{
		offer = (fileoffer *) list->data;
		if(strcasecmp(name, offer->file) == 0)
		{
			file_list = g_slist_remove(file_list, offer);
			xchat_printf(ph, "%s [%s] removed.\n", offer->file, offer->fullpath);
			free(offer->file);
			free(offer->desc);
			free(offer->fullpath);
			free(offer);
			return;
		}
		list = list->next;
	}
}

static void xdcc_add(char *name, char *fullpath, char *desc, int dl)
{
	fileoffer *offer;

	offer = (fileoffer *) malloc(sizeof(fileoffer));
	offer->file = strdup(name);
	offer->desc = strdup(desc);
	offer->fullpath = strdup(fullpath);
	offer->downloads = dl;

	file_list = g_slist_append(file_list, offer);
}

static void xdcc_list(char *nick, char *host, char *arg, char *cmd)
{
	GSList *list;
	int i = 0;
	fileoffer *offer;

	xchat_commandf(ph, "%s %s :XDCC List:", cmd, nick);
	list = file_list;
	while(list)
	{
		i++;
		offer = (fileoffer *) list->data;
		xchat_commandf(ph, "%s %s :[#%d] %s - %s [%d dl]", cmd,
							nick, i, offer->file, offer->desc, offer->downloads);
		list = list->next;
	}

	if(i == 0)
		xchat_commandf(ph, "%s %s :- list empty.", cmd, nick);
	else
		xchat_commandf(ph, "%s %s :%d files listed.", cmd, nick, i);
}

static int xdcc_command(char *word[], char *word_eol[], void *userdata)
{
	if(strcasecmp(word[2], "ADD") == 0)
	{
		if(!word_eol[5][0])
			xchat_print(ph, "Syntax: /XDCC ADD <name> <path> <description>\n");
		else
		{
			if(access(word[4], R_OK) == 0)
			{
				xdcc_add(word[3], word[4], word_eol[5], 0);
				xchat_printf(ph, "%s [%s] added.\n", word[3], word[4]);
			}
			else
				xchat_printf(ph, "Cannot read %s\n", word[4]);
		}
		return EAT_XCHAT;
	}

	if(strcasecmp(word[2], "DEL") == 0)
	{
		xdcc_del(word[3]);
		return EAT_XCHAT;
	}

	if(strcasecmp(word[2], "SLOTS") == 0)
	{
		if(word[3][0])
		{
			xdcc_slots = atoi(word[3]);
			xchat_printf(ph, "XDCC slots set to %d\n", xdcc_slots);
		} else
		{
			xchat_printf(ph, "XDCC slots: %d\n", xdcc_slots);
		}
		return EAT_XCHAT;
	}

	if(strcasecmp(word[2], "ON") == 0)
	{
		xdcc_on = TRUE;
		xchat_print(ph, "XDCC now ON\n");
		return EAT_XCHAT;
	}

	if(strcasecmp(word[2], "LIST") == 0)
	{
		xdcc_list("", "", "", "echo");
		return EAT_XCHAT;
	}

	if(strcasecmp(word[2], "OFF") == 0)
	{
		xdcc_on = FALSE;
		xchat_print(ph, "XDCC now OFF\n");
		return EAT_XCHAT;
	}

	xchat_print(ph, "Syntax: XDCC ADD <name> <fullpath> <description>\n"
						 "        XDCC DEL <name>\n"
						 "        XDCC SLOTS <number>\n"
						 "        XDCC LIST\n"
						 "        XDCC ON\n"
						 "        XDCC OFF\n\n");

	return EAT_XCHAT;
}

static void xdcc_remote(char *from, char *msg)
{
  	char *ex, *nick, *host;

	ex = strchr(from, '!');
	if(!ex)
		return;
	ex[0] = 0;
	nick = from;
	host = ex + 1;

	if(xdcc_on == 0)
	{
		xchat_commandf(ph, "notice %s XDCC is turned OFF!", from);
		return;
	}

	if(strncasecmp(msg, "LIST", 4) == 0)
		xdcc_list(nick, host, msg + 4, "quote notice");
	else if(strncasecmp(msg, "GET ", 4) == 0)
		xdcc_get(nick, host, msg + 4);
	else
		xchat_commandf(ph, "notice %s Unknown XDCC command!", from);
}

static int ctcp_cb(char *word[], void *userdata)
{
  	char *msg = word[1];
	char *from = word[2];

	if(strncasecmp(msg, "XDCC ", 5) == 0)
		xdcc_remote(from, msg + 5);

	return EAT_NONE;
}

static void xdcc_save(void)
{
	char buf[512];
	FILE *fp;
	GSList *list;
	fileoffer *offer;

	snprintf(buf, sizeof(buf), "%s/xdcclist.conf", xchat_get_info(ph, "xchatdir"));

	fp = fopen(buf, "w");
	if(!fp)
		return;

	list = file_list;
	while(list)
	{
		offer = (fileoffer *) list->data;
		fprintf(fp, "%s\n%s\n%s\n%d\n\n\n", offer->file, offer->fullpath,
							 offer->desc, offer->downloads);
		list = list->next;
	}

	fclose(fp);
}

static void xdcc_load(void)
{
	char buf[512];
	char file[128];
	char path[128];
	char desc[128];
	char dl[128];
	FILE *fp;

	snprintf(buf, sizeof(buf), "%s/xdcclist.conf", xchat_get_info(ph, "xchatdir"));

	fp = fopen(buf, "r");
	if(!fp)
		return;

	while(fgets(file, sizeof(file), fp))
	{
		file[strlen(file)-1] = 0;
		fgets(path, sizeof(path), fp);
		path[strlen(path)-1] = 0;
		fgets(desc, sizeof(desc), fp);
		desc[strlen(desc)-1] = 0;
		fgets(dl, sizeof(dl), fp);
		dl[strlen(dl)-1] = 0;
		fgets(buf, sizeof(buf), fp);
		fgets(buf, sizeof(buf), fp);
		xdcc_add(file, path, desc, atoi(dl));
	}

	fclose(fp);
}

int xchat_plugin_deinit(void)
{
	xdcc_save();
	xchat_print(ph, "XDCC List saved\n");
	return 1;
}

int xchat_plugin_init(xchat_plugin *plugin_handle,
				char **plugin_name, char **plugin_desc, char **plugin_version,
				char *arg)
{
	ph = plugin_handle;

	*plugin_name = "XDCC";
	*plugin_desc = "Very simple XDCC server";
	*plugin_version = "0.1";

	xchat_hook_command(ph, "XDCC", PRI_NORM, xdcc_command, 0, 0);
	xchat_hook_print(ph, "CTCP Generic", PRI_NORM, ctcp_cb, 0);
	xchat_hook_print(ph, "CTCP Generic to Channel", PRI_NORM, ctcp_cb, 0);

	xdcc_load();
	xchat_print(ph, "XDCC loaded. Type /XDCC for help.\n");

	return 1;
}
