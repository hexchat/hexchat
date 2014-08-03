/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING or http://lwsitu.com/xchat/COPYING
 * for more details. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hexchat-plugin.h"

#if defined(_WIN32) || defined(_WIN64)
#define strtok_r strtok_s
#endif
static hexchat_plugin *ph;

static int
parse_command(const char *const word[], const char *const word_eol[], void *userdata) {
	char *channel = NULL, *server = NULL, *token = NULL;
	char *save_ptr1 = NULL;
	char *str1 = NULL;
	char *delimiter = NULL;

	hexchat_context *ctx = NULL;

	if( word[2] != NULL && word[3] != NULL ) {
		for( str1 = strdup(word[2]); ; str1 = NULL ) {
			token = strtok_r( str1, ",", &save_ptr1 );
			/*token = strtok( str1, "," );*/
/*			printf( "token: %s\n", token );*/

			if( token == NULL ) {
				break;
			}

			channel = strdup( token );
				
			delimiter = strchr( channel, '/' );

			server = NULL;
			if( delimiter != NULL ) {
				*delimiter = '\0';

				if( strlen( delimiter + 1 ) > 0 ) {
					server = strdup( delimiter + 1 );
				}
			}

			/* /Network form */
			if( strlen( channel ) == 0 ) {
				free( channel );
				channel = NULL;
			}

/*			printf( "channel[%s] server[%s]\n", channel, server );*/

			if( (ctx = hexchat_find_context( ph, server, channel ) ) != NULL ) {
				if( hexchat_set_context( ph, ctx ) ) {
					hexchat_command( ph, word_eol[3] );
				}
			}

			if( channel != NULL ) {
				free( channel );
			}

			if( server != NULL ) {
				free( server );
			}
		}
		free(str1);
	}
	return HEXCHAT_EAT_HEXCHAT;
}

int
hexchat_plugin_init( hexchat_plugin * plugin_handle, char **plugin_name,
	char **plugin_desc, char **plugin_version, char *arg ) {

	ph = plugin_handle;
	*plugin_name = "Do At";
	*plugin_version = "1.0001";
	*plugin_desc = "Perform an arbitrary command on multiple channels";

	hexchat_hook_command( ph, "doat", HEXCHAT_PRI_NORM, parse_command, "DOAT [channel,list,/network] [command], perform a command on multiple contexts", NULL );

	hexchat_print (ph, "Do At plugin loaded\n");

	return 1;
}

int
hexchat_plugin_deinit (void)
{
	hexchat_print (ph, "Do At plugin unloaded\n");
	return 1;
}
