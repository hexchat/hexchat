/***************************************************************************
                           tclplugin.h  -  TCL plugin header file
                           -------------------------------------------------
    begin                : Sat Nov  9 17:31:20 MST 2002
    copyright            : Copyright 2002-2005 Daniel P. Stasinski
    email                : mooooooo@avenues.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define BADARGS(nl,nh,example) \
    if ((argc<(nl)) || (argc>(nh))) { \
    Tcl_AppendResult(irp,"wrong # args: should be \"",argv[0], \
        (example),"\"",NULL); \
        return TCL_ERROR; \
    }

#define CHECKCTX(ctx) \
    if (ctx == NULL) { \
        Tcl_AppendResult(irp, "No such server/channel/nick", NULL); \
        return TCL_ERROR; \
    }

typedef struct {
    char *procPtr;
    xchat_hook *hook;
} alias;

typedef struct {
    int timerid;
    time_t timestamp;
    char *procPtr;
    int count;
    int seconds;
} timer;

typedef struct {
    int result;
    int defresult;
} t_complete;

#define MAX_TIMERS 256
#define MAX_COMPLETES 128

static char *StrDup(const char *string, int *length);
static char *myitoa(long value);
static xchat_context *xchat_smart_context(const char *arg1, const char *arg2);
static void queue_nexttimer();
static int insert_timer(int seconds, int count, const char *script);
static void do_timer();
static int Server_raw_line(char *word[], char *word_eol[], void *userdata);
static int Print_Hook(char *word[], void *userdata);
static int tcl_timerexists(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_killtimer(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_timers(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_timer(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_on(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_off(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_alias(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_complete(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_raw(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_command(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_xchat_puts(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_print(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_channels(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_servers(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_queries(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_users(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_chats(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_ignores(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_dcclist(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_me(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_xchat_nickcmp(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_strip(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_topic(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int tcl_notifylist(ClientData cd, Tcl_Interp * irp, int argc, const char *argv[]);
static int Command_Alias(char *word[], char *word_eol[], void *userdata);
static int Null_Command_Alias(char *word[], char *word_eol[], void *userdata);
static int Command_TCL(char *word[], char *word_eol[], void *userdata);
static int Command_Source(char *word[], char *word_eol[], void *userdata);
static int Command_Reload(char *word[], char *word_eol[], void *userdata);
static int TCL_Event_Handler(void *userdata);
static void Tcl_Plugin_Init();
static void Tcl_Plugin_DeInit();
static void banner();
int xchat_plugin_init(xchat_plugin * plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg);
int xchat_plugin_deinit();
