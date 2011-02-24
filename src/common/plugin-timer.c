#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "xchat-plugin.h"

#ifdef WIN32
#define strcasecmp stricmp
#endif

static xchat_plugin *ph;	/* plugin handle */
static GSList *timer_list = NULL;

#define STATIC
#define HELP \
"Usage: TIMER [-refnum <num>] [-repeat <num>] <seconds> <command>\n" \
"       TIMER [-quiet] -delete <num>"

typedef struct
{
	xchat_hook *hook;
	xchat_context *context;
	char *command;
	int ref;
	int repeat;
	float timeout;
	unsigned int forever:1;
} timer;

static void
timer_del (timer *tim)
{
	timer_list = g_slist_remove (timer_list, tim);
	free (tim->command);
	xchat_unhook (ph, tim->hook);
	free (tim);
}

static void
timer_del_ref (int ref, int quiet)
{
	GSList *list;
	timer *tim;

	list = timer_list;
	while (list)
	{
		tim = list->data;
		if (tim->ref == ref)
		{
			timer_del (tim);
			if (!quiet)
				xchat_printf (ph, "Timer %d deleted.\n", ref);
			return;
		}
		list = list->next;
	}
	if (!quiet)
		xchat_print (ph, "No such ref number found.\n");
}

static int
timeout_cb (timer *tim)
{
	if (xchat_set_context (ph, tim->context))
	{
		xchat_command (ph, tim->command);

		if (tim->forever)
			return 1;

		tim->repeat--;
		if (tim->repeat > 0)
			return 1;
	}

	timer_del (tim);
	return 0;
}

static void
timer_add (int ref, float timeout, int repeat, char *command)
{
	timer *tim;
	GSList *list;

	if (ref == 0)
	{
		ref = 1;
		list = timer_list;
		while (list)
		{
			tim = list->data;
			if (tim->ref >= ref)
				ref = tim->ref + 1;
			list = list->next;
		}
	}

	tim = malloc (sizeof (timer));
	tim->ref = ref;
	tim->repeat = repeat;
	tim->timeout = timeout;
	tim->command = strdup (command);
	tim->context = xchat_get_context (ph);
	tim->forever = FALSE;

	if (repeat == 0)
		tim->forever = TRUE;

	tim->hook = xchat_hook_timer (ph, timeout * 1000.0, (void *)timeout_cb, tim);
	timer_list = g_slist_append (timer_list, tim);
}

static void
timer_showlist (void)
{
	GSList *list;
	timer *tim;

	if (timer_list == NULL)
	{
		xchat_print (ph, "No timers installed.\n");
		xchat_print (ph, HELP);
		return;
	}
							 /*  00000 00000000 0000000 abc */
	xchat_print (ph, "\026 Ref#  Seconds  Repeat  Command \026\n");
	list = timer_list;
	while (list)
	{
		tim = list->data;
		xchat_printf (ph, "%5d %8.1f %7d  %s\n", tim->ref, tim->timeout,
						  tim->repeat, tim->command);
		list = list->next;
	}
}

static int
timer_cb (char *word[], char *word_eol[], void *userdata)
{
	int repeat = 1;
	float timeout;
	int offset = 0;
	int ref = 0;
	int quiet = FALSE;
	char *command;

	if (!word[2][0])
	{
		timer_showlist ();
		return XCHAT_EAT_XCHAT;
	}

	if (strcasecmp (word[2], "-quiet") == 0)
	{
		quiet = TRUE;
		offset++;
	}

	if (strcasecmp (word[2 + offset], "-delete") == 0)
	{
		timer_del_ref (atoi (word[3 + offset]), quiet);
		return XCHAT_EAT_XCHAT;
	}

	if (strcasecmp (word[2 + offset], "-refnum") == 0)
	{
		ref = atoi (word[3 + offset]);
		offset += 2;
	}

	if (strcasecmp (word[2 + offset], "-repeat") == 0)
	{
		repeat = atoi (word[3 + offset]);
		offset += 2;
	}

	timeout = atof (word[2 + offset]);
	command = word_eol[3 + offset];

	if (timeout < 0.1 || !command[0])
		xchat_print (ph, HELP);
	else
		timer_add (ref, timeout, repeat, command);

	return XCHAT_EAT_XCHAT;
}

int
#ifdef STATIC
timer_plugin_init
#else
xchat_plugin_init
#endif
				(xchat_plugin *plugin_handle, char **plugin_name,
				char **plugin_desc, char **plugin_version, char *arg)
{
	/* we need to save this for use with any xchat_* functions */
	ph = plugin_handle;

	*plugin_name = "Timer";
	*plugin_desc = "IrcII style /TIMER command";
	*plugin_version = "";

	xchat_hook_command (ph, "TIMER", XCHAT_PRI_NORM, timer_cb, HELP, 0);

	return 1;       /* return 1 for success */
}
