/* XCHAT 2.0 PLUGIN: Mail checker */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "xchat-plugin.h"


static xchat_plugin *ph;	/* plugin handle */

static int
mail_items(char *file)
{
	FILE *fp;
	int items;
	char buf[512];

	fp = fopen(file, "r");
	if(!fp)
		return 1;

	items = 0;
	while(fgets(buf, sizeof buf, fp))
	{
		if(!strncmp(buf, "From ", 5))
			items++;
	}
	fclose(fp);

	return items;
}

static void
xchat_mail_check (void)
{
	static int last_size = -1;
	int size;
	struct stat st;
	char buf[512];
	char *maildir;

	maildir = getenv("MAIL");
	if(!maildir)
	{
		snprintf (buf, sizeof(buf), "/var/spool/mail/%s", getenv("USER"));
		maildir = buf;
	}

	if(stat(maildir, &st) < 0)
		return;

	size = st.st_size;

	if(last_size == -1)
	{
		last_size = size;
		return;
	}

	if(size > last_size)
	{
		xchat_printf(ph,
	"-\0033-\0039-\017\tYou have new mail (%d messages, %d bytes total).",
				mail_items(maildir), size);
	}

	last_size = size;
}

static int timeout_cb(void *userdata)
{
	xchat_mail_check();

	return 1;
}

int xchat_plugin_init(xchat_plugin *plugin_handle,
				char **plugin_name, char **plugin_desc, char **plugin_version,
				char *arg)
{
	ph = plugin_handle;

	*plugin_name = "MailCheck";
	*plugin_desc = "Checks your mailbox every 30 seconds";
	*plugin_version = "0.1";

	xchat_hook_timer(ph, 30000, timeout_cb, 0);

	return 1;
}
