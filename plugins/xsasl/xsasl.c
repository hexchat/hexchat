/*
 * SASL authentication plugin for XChat
 * Extremely primitive: only PLAIN, no error checking
 *
 * Copyright (c) 2010, <ygrek@autistici.org>
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

#define PNAME "XSASL"
#define PDESC "SASL authentication plugin";
#define PVERSION "0.0.4"

static xchat_plugin *ph;   /* plugin handle */

struct sasl_info;

struct sasl_info
{
  char const* login;
  char const* password;
  char const* network;
  struct sasl_info* next;
};

typedef struct sasl_info sasl_info;

sasl_info* all_info = NULL;

static void add_info(char const* login, char const* password, char const* network)
{
  sasl_info* prev = all_info;
  sasl_info* info = (sasl_info*)malloc(sizeof(sasl_info));

  info->login = strdup(login);
  info->password = strdup(password);
  info->network = strdup(network);
  info->next = prev;

  all_info = info;
}

static sasl_info* find_info(char const* network)
{
  sasl_info* cur = all_info;
  while (cur)
  {
    if (0 == strcmp(cur->network, network)) return cur;
    cur = cur->next;
  }
  return NULL;
}

static sasl_info* get_info(void)
{
  const char* name = xchat_get_info(ph, "network");
  if (name) 
    return find_info(name);
  else 
    return NULL;
}

static int authend_cb(char *word[], char *word_eol[], void *userdata)
{
  if (get_info())
  {
    xchat_printf(ph, "XSASL result: %s", word_eol[1]);
    xchat_commandf(ph, "QUOTE CAP END");
  }
  return XCHAT_EAT_ALL;
}

/*
static int disconnect_cb(char *word[], void *userdata)
{
  xchat_printf(ph, "disconnected");
  return XCHAT_EAT_NONE;
}
*/

static int server_cb(char *word[], char *word_eol[], void *userdata)
{
  if (0 == strcmp("AUTHENTICATE",word[1]) && 0 == strcmp("+",word[2]))
  {
    size_t len;
    char* buf;
    char* enc;
    sasl_info* p = get_info();
    if (!p) return XCHAT_EAT_NONE;

    xchat_printf(ph,"XSASL authenticating as %s",p->login);

    len = strlen(p->login)*2 + 2 + strlen(p->password);
    buf = (char*)malloc(len + 1);
    strcpy(buf,p->login);
    strcpy(buf+strlen(p->login)+1,p->login);
    strcpy(buf+strlen(p->login)*2+2,p->password);
    enc = g_base64_encode((unsigned char*)buf,len);

    /*xchat_printf(ph,"AUTHENTICATE %s",enc);*/
    xchat_commandf(ph,"QUOTE AUTHENTICATE %s",enc);

    free(enc);
    free(buf);

    return XCHAT_EAT_ALL;
  }
	return XCHAT_EAT_NONE;
}

static int cap_cb(char *word[], char *word_eol[], void *userdata)
{
  if (get_info())
  {
    /* FIXME test sasl cap */
    xchat_printf(ph, "XSASL info: %s", word_eol[1]);
    xchat_commandf(ph,"QUOTE AUTHENTICATE PLAIN");
  }

  return XCHAT_EAT_ALL;
}

static int sasl_cmd_cb(char *word[], char *word_eol[], void *userdata)
{
  const char* login = word[2];
  const char* password = word[3];
  const char* network = word_eol[4];

  if (!login || !*login)
  {
    sasl_info *cur = all_info;
    if (NULL == cur)
    {
      xchat_printf(ph,"Nothing, see /help sasl");
      return XCHAT_EAT_ALL;
    }

    while (cur)
    {
      xchat_printf(ph,"%s:%s at %s",cur->login,cur->password,cur->network);
      cur = cur->next;
    }
    return XCHAT_EAT_ALL;
  }

  if (!login || !password || !network || !*login || !*password || !*network)
  {
    xchat_printf(ph,"Wrong usage, try /help sasl");
    return XCHAT_EAT_ALL;
  }

  add_info(login,password,network);

  xchat_printf(ph,"Enabled SASL authentication for %s",network);

  return XCHAT_EAT_ALL;
}

static int connect_cb(char *word[], void *userdata)
{
  if (get_info())
  {
    xchat_printf(ph, "XSASL enabled");
    xchat_commandf(ph, "QUOTE CAP REQ :sasl");
  }

  return XCHAT_EAT_NONE;
}

int xchat_plugin_init(xchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
  /* we need to save this for use with any xchat_* functions */
  ph = plugin_handle;

  /* tell xchat our info */
  *plugin_name = PNAME;
  *plugin_desc = PDESC;
  *plugin_version = PVERSION;

  xchat_hook_command(ph, "sasl", XCHAT_PRI_NORM, sasl_cmd_cb, 
    "Usage: SASL <login> <password> <network>, enable SASL authentication for given network", 0);

  xchat_hook_print(ph, "Connected", XCHAT_PRI_NORM, connect_cb, NULL);
/*
  xchat_hook_print(ph, "Disconnected", XCHAT_PRI_NORM, disconnect_cb, NULL);
*/

  xchat_hook_server(ph, "CAP", XCHAT_PRI_NORM, cap_cb, NULL);
  xchat_hook_server(ph, "RAW LINE", XCHAT_PRI_NORM, server_cb, NULL);

  xchat_hook_server(ph, "903", XCHAT_PRI_NORM, authend_cb, NULL);
  xchat_hook_server(ph, "904", XCHAT_PRI_NORM, authend_cb, NULL);
  xchat_hook_server(ph, "905", XCHAT_PRI_NORM, authend_cb, NULL);
  xchat_hook_server(ph, "906", XCHAT_PRI_NORM, authend_cb, NULL);
  xchat_hook_server(ph, "907", XCHAT_PRI_NORM, authend_cb, NULL);

  /* xchat_print(ph,"Loading cap_sasl.conf");
  xchat_commandf(ph, "load -e %s/cap_sasl.conf",xchat_get_info(ph, "xchatdir")); */

  xchat_printf(ph, PNAME " plugin loaded\n");

  return 1;
}

int xchat_plugin_deinit (void)
{
	xchat_printf(ph, PNAME " plugin unloaded\n");
	return 1;
}
