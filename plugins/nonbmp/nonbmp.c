#include <stdlib.h>
#include <glib.h>
#include <string.h>

#include "xchat-plugin.h"

static xchat_plugin *ph;
static const char name[] = "Non-BMP";
static const char desc[] = "Replace non-BMP characters with replacement characters";
static const char version[] = "1.0000";
static int recursing = 0;

static int filter(
	char *word[],
	char *word_eol[],
	void *unused
) {
	gunichar *line;
	gchar *utf8_line;
	glong length;
	glong index;

	if( recursing ) {
		return XCHAT_EAT_NONE;
	}

	/* the input has already been checked so we can use the _fast version */
	line = g_utf8_to_ucs4_fast(
		(char *)word_eol[1],
		-1, /* NUL terminated input */
		&length
	);
	
	for( index = 0; index < length; index++ ) {
		if( line[ index ] > 0xFFFF ) {
			line[ index ] = 0xFFFD; /* replacement character */
		}
	}

	utf8_line = g_ucs4_to_utf8(
		line,
		-1, /* NUL terminated input */
		NULL, /* items read */
		NULL, /* items written */
		NULL  /* ignore conversion error */
	);

	if( utf8_line == NULL ) {
		/* conversion failed ... I guess we are screwed? */
		g_free( line );
		return XCHAT_EAT_NONE;
	}

	recursing = 1;
	xchat_commandf( ph, "RECV %s", utf8_line );
	recursing = 0;

	g_free( line );
	g_free( utf8_line );
	return XCHAT_EAT_ALL;
}

int xchat_plugin_init(
	xchat_plugin *plugin_handle,
	char **plugin_name,
	char **plugin_desc,
	char **plugin_version,
	char *arg 
) {
/*	int index = 0;*/

	ph = plugin_handle;
	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	xchat_hook_server( ph, "RAW LINE", XCHAT_PRI_HIGHEST, filter, (void *)NULL );
	xchat_printf (ph, "%s plugin loaded\n", name);
	return 1;
}

int
xchat_plugin_deinit (void)
{
	xchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}