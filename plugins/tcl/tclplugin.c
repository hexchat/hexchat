/***************************************************************************
                          tclplugin.c  -  Tcl plugin for xchat 1.9.x / 2.x.x
                          --------------------------------------------------
    begin                : Sat Nov 19 17:31:20 MST 2002
    copyright            : Copyright 2002-2003 Daniel P. Stasinski
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

#define VERSION "1.0.5"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#include <tcl.h>
#include <tclDecls.h>
#include <sys/stat.h>

#include "xchat-plugin.h"
#include "tclplugin.h"
#include "printevents.h"

static int nexttimerid = 0;
static int nexttimerindex = 0;
static timer timers[MAX_TIMERS];

static int initialized = 0;
static int reinit_tried = 0;
static Tcl_Interp *interp = NULL;
static xchat_plugin *ph;
static xchat_hook *raw_line_hook;
static xchat_hook *Command_TCL_hook;
static xchat_hook *Command_Source_hook;
static xchat_hook *Command_Rehash_hook;
static xchat_hook *Command_Load_hook;
static xchat_hook *Event_Handler_hook;

static int complete = 0;
static Tcl_HashTable cmdTablePtr;
static Tcl_HashTable aliasTablePtr;

static char unknown[] = {
    "proc ::unknown {args} {\n"
        "global errorInfo errorCode\n"
        "set cmd [lindex $args 0]\n" "if { [string index $cmd 0] == \"/\" } {\n" "command \"[string range $cmd 1 end] [join [lrange $args 1 end] \" \"]\"\n" "} else {\n" "return -code error \"invalid command name \\\"$cmd\\\"\"\n" "}\n" "}"
};

static char sourcedirs[] = {
    "set files [lsort [glob [xchatdir]/*.tcl]]\n"
        "set init [lsearch -glob $files \"*/init.tcl\"]\n"
        "if { $init > 0 } {\n"
        "set initfile [lindex $files $init]\n"
        "set files [lreplace $files $init $init]\n"
        "set files [linsert $files 0 $initfile]\n" "}\n" "foreach f $files {\n" "if { [catch { source $f } errMsg] } {\n" "puts \"Tcl plugin\\tError sourcing \\\"$f\\\" ($errMsg)\"\n" "} else {\n" "puts \"Tcl plugin\\tSourced \\\"$f\\\"\"\n" "}\n" "}\n"
};

static char *StrDup(char *string, int *length)
{
    char *result;
    if ((*length = strlen(string)) == 0)
        return NULL;
    result = Tcl_Alloc(*length + 1);
    strncpy(result, string, *length + 1);
    return result;
}

static char *myitoa(int value)
{
    static char result[32];
    sprintf(result, "%d", value);
    return result;
}

static xchat_context *xchat_smart_context(char *arg1, char *arg2)
{
    /* slightly tweak xchat_find_context's amazing psychic abilities */

    char *server = NULL;
    char *channel = NULL;

    if (arg1) {
        if (strchr(arg1, '.'))
            server = arg1;
        else
            channel = arg1;
    }

    if (arg2) {
        if (strchr(arg2, '.'))
            server = arg2;
        else
            channel = arg2;
    }

    if (server && channel)
        return xchat_find_context(ph, server, channel);
    else if (server && !channel)
        return xchat_find_context(ph, server, server);
    else if (!server && channel)
        return xchat_find_context(ph, (char *) xchat_get_info(ph, "server"), channel);
    else
        return xchat_get_context(ph);

}

static void queue_nexttimer()
{
    int x;
    time_t then;

    nexttimerindex = 0;
    then = (time_t) 9999999999;

    for (x = 1; x < MAX_TIMERS; x++) {
        if (timers[x].timerid) {
            if (timers[x].timestamp < then) {
                then = timers[x].timestamp;
                nexttimerindex = x;
            }
        }
    }
}

static int insert_timer(int seconds, char *script)
{
    int x;
    int dummy;
    time_t now;

    if (script == NULL)
        return (-1);

    now = time(NULL);

    for (x = 1; x < MAX_TIMERS; x++) {
        if (timers[x].timerid == 0) {
            timers[x].timerid = (nexttimerid++ % INT_MAX) + 1;
            timers[x].timestamp = now + seconds;
            timers[x].procPtr = StrDup(script, &dummy);
            queue_nexttimer();
            return (timers[x].timerid);
        }
    }

    return (-1);
}

static void do_timer()
{
    xchat_context *origctx;
    time_t now;
    int index;

    if (!nexttimerindex)
        return;

    now = time(NULL);

    if (now < timers[nexttimerindex].timestamp)
        return;

    index = nexttimerindex;

    origctx = xchat_get_context(ph);
    if (Tcl_Eval(interp, timers[index].procPtr) == TCL_ERROR) {
        xchat_printf(ph, "\0039Tcl plugin:\003\tTIMER ERROR %s \n", Tcl_GetStringResult(interp));
    }
    xchat_set_context(ph, origctx);

    timers[index].timerid = 0;
    if (timers[index].procPtr != NULL)
        Tcl_Free(timers[index].procPtr);
    timers[index].procPtr = NULL;

    queue_nexttimer();

    return;

}

static int Server_raw_line(char *word[], char *word_eol[], void *userdata)
{
    char *src, *cmd, *dest, *rest;
    char *chancmd = NULL;
    char *procList;
    Tcl_HashEntry *entry = NULL;
    xchat_context *origctx;
    int len;
    int dummy;
    char *string = NULL;
    int ctcp = 0;
    char *label = NULL;
    int count;
    int list_argc, proc_argc;
    char **list_argv, **proc_argv;
    int private = 0;

    complete = XCHAT_EAT_NONE;

    if (word[1][0] == ':') {
        src = word[1];
        cmd = word[2];
        dest = word[3];
        rest = word_eol[4];
    } else {
        src = "";
        cmd = word[1];
        if (word_eol[2][0] == ':') {
            dest = "";
            rest = word_eol[2];
        } else {
            dest = word[2];
            rest = word_eol[3];
        }
    }

    if (src[0] == ':')
        src++;
    if (dest[0] == ':')
        dest++;
    if (rest[0] == ':')
        rest++;

    if (rest[0] == 0x01) {
        rest++;
        if (strcasecmp("PRIVMSG", cmd) == 0) {
            if (strncasecmp(rest, "ACTION ", 7) == 0) {
                cmd = "ACTION";
                rest += 7;
            } else
                cmd = "CTCP";
        } else if (!strcasecmp("NOTICE", cmd))
            cmd = "CTCR";
        ctcp = 1;
    } else if (!strcasecmp("NOTICE", cmd) && (strchr(src, '!') == NULL)) {
        cmd = "SNOTICE";
    } else if (rest[0] == '!') {
        chancmd = word[4] + 1;
    }

    if (chancmd != NULL) {
        string = StrDup(chancmd, &dummy);
        Tcl_UtfToUpper(string);
        if ((entry = Tcl_FindHashEntry(&cmdTablePtr, string)) == NULL) {
            Tcl_Free(string);
        } else {
            cmd = chancmd;
            rest = word_eol[5];
        }
    }

    if (entry == NULL) {
        string = StrDup(cmd, &dummy);
        Tcl_UtfToUpper(string);
        entry = Tcl_FindHashEntry(&cmdTablePtr, string);
    }

    if (entry != NULL) {

        procList = Tcl_GetHashValue(entry);

        if (isalpha(dest[0]))
            private = 1;

        rest = StrDup(rest, &len);
        if (ctcp) {
            if (rest != NULL) {
                if ((len > 1) && (rest[len - 1] == 0x01))
                    rest[len - 1] = 0;
            }
        }

        if (Tcl_SplitList(interp, procList, &list_argc, &list_argv) == TCL_OK) {

            for (count = 0; count < list_argc; count++) {

                if (Tcl_SplitList(interp, list_argv[count], &proc_argc, &proc_argv) != TCL_OK)
                    continue;

                label = proc_argv[0];

                Tcl_SetVar(interp, "_label", label, TCL_NAMESPACE_ONLY);
                Tcl_SetVar(interp, "_private", myitoa(private), TCL_NAMESPACE_ONLY);
                Tcl_SetVar(interp, "_raw", word_eol[1], TCL_NAMESPACE_ONLY);
                Tcl_SetVar(interp, "_src", src, TCL_NAMESPACE_ONLY);
                Tcl_SetVar(interp, "_cmd", cmd, TCL_NAMESPACE_ONLY);
                Tcl_SetVar(interp, "_dest", dest, TCL_NAMESPACE_ONLY);

                if (rest == NULL)
                    Tcl_SetVar(interp, "_rest", "", TCL_NAMESPACE_ONLY);
                else
                    Tcl_SetVar(interp, "_rest", rest, TCL_NAMESPACE_ONLY);

                origctx = xchat_get_context(ph);
                if (Tcl_Eval(interp, proc_argv[1]) == TCL_ERROR)
                    xchat_printf(ph, "\0039Tcl plugin:\003\tERROR (%s:%s) %s \n", cmd, label, Tcl_GetStringResult(interp));
                xchat_set_context(ph, origctx);

                Tcl_Free((char *) proc_argv);

                if (complete)
                    break;

            }

            Tcl_Free((char *) list_argv);

        }

        Tcl_Free(rest);

    }

    if (string)
        Tcl_Free(string);

    return complete;            /* see 'tcl_complete */

}

static int Print_Hook(char *word[], void *userdata)
{
    char *procList;
    Tcl_HashEntry *entry;
    xchat_context *origctx;
    int count;
    char *label;
    int list_argc, proc_argc;
    char **list_argv, **proc_argv;
    Tcl_DString ds;
    int x;

    complete = XCHAT_EAT_NONE;

    if ((entry = Tcl_FindHashEntry(&cmdTablePtr, xc[(int) userdata].event)) != NULL) {

        procList = Tcl_GetHashValue(entry);

        if (Tcl_SplitList(interp, procList, &list_argc, &list_argv) == TCL_OK) {

            for (count = 0; count < list_argc; count++) {

                if (Tcl_SplitList(interp, list_argv[count], &proc_argc, &proc_argv) != TCL_OK)
                    continue;

                label = proc_argv[0];

                Tcl_DStringInit(&ds);
                if ((int) userdata == CHAT) {
                    Tcl_DStringAppend(&ds, word[3], strlen(word[3]));
                    Tcl_DStringAppend(&ds, "!*@", 3);
                    Tcl_DStringAppend(&ds, word[1], strlen(word[1]));
                    Tcl_SetVar(interp, "_src", ds.string, TCL_NAMESPACE_ONLY);
                    Tcl_SetVar(interp, "_dest", word[2], TCL_NAMESPACE_ONLY);
                    Tcl_SetVar(interp, "_rest", word[4], TCL_NAMESPACE_ONLY);
                    Tcl_SetVar(interp, "_raw", "", TCL_NAMESPACE_ONLY);
                } else {
                    if (xc[(int) userdata].argc > 0) {
                        for (x = 0; x <= xc[(int) userdata].argc; x++)
                            Tcl_DStringAppendElement(&ds, word[x]);
                    }
                    Tcl_SetVar(interp, "_raw", ds.string, TCL_NAMESPACE_ONLY);
                }
                Tcl_DStringFree(&ds);

                Tcl_SetVar(interp, "_label", label, TCL_NAMESPACE_ONLY);
                Tcl_SetVar(interp, "_cmd", xc[(int) userdata].event, TCL_NAMESPACE_ONLY);

                origctx = xchat_get_context(ph);
                if (Tcl_Eval(interp, proc_argv[1]) == TCL_ERROR) {
                    xchat_printf(ph, "\0039Tcl plugin:\003\tERROR (%s:%s) %s \n", xc[(int) userdata].event, label, Tcl_GetStringResult(interp));
                }
                xchat_set_context(ph, origctx);

                Tcl_Free((char *) proc_argv);

                if (complete)
                    break;

            }

            Tcl_Free((char *) list_argv);

        }
    }

    return complete;
}


static int tcl_timerexists(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    int x;
    int timerid;

    BADARGS(2, 2, " schedid");

    if (Tcl_GetInt(irp, argv[1], &timerid) != TCL_OK) {
        Tcl_AppendResult(irp, "Invalid timer id", NULL);
        return TCL_ERROR;
    }

    if (timerid) {
        for (x = 1; x < MAX_TIMERS; x++) {
            if (timers[x].timerid == timerid) {
                Tcl_AppendResult(irp, "1", NULL);
                return TCL_OK;
            }
        }
    }

    Tcl_AppendResult(irp, "0", NULL);

    return TCL_OK;
}

static int tcl_killtimer(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    int x;
    int timerid;

    BADARGS(2, 2, " timerid");

    if (Tcl_GetInt(irp, argv[1], &timerid) != TCL_OK) {
        Tcl_AppendResult(irp, "Invalid timer id", NULL);
        return TCL_ERROR;
    }

    if (timerid) {
        for (x = 1; x < MAX_TIMERS; x++) {
            if (timers[x].timerid == timerid) {
                timers[x].timerid = 0;
                if (timers[x].procPtr != NULL)
                    Tcl_Free(timers[x].procPtr);
                timers[x].procPtr = NULL;
                break;
            }
        }
    }

    queue_nexttimer();

    return TCL_OK;
}

static int tcl_timers(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    int x;
    Tcl_DString ds;
    time_t now;

    BADARGS(1, 1, "");

    now = time(NULL);

    Tcl_DStringInit(&ds);

    for (x = 1; x < MAX_TIMERS; x++) {
        if (timers[x].timerid) {
            Tcl_DStringStartSublist(&ds);
            Tcl_DStringAppendElement(&ds, myitoa(timers[x].timerid));
            Tcl_DStringAppendElement(&ds, myitoa(timers[x].timestamp - now));
            Tcl_DStringAppendElement(&ds, timers[x].procPtr);
            Tcl_DStringEndSublist(&ds);
        }
    }

    Tcl_AppendResult(interp, ds.string, NULL);
    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_timer(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    int seconds;
    int timerid;
    char reply[32];

    BADARGS(3, 3, " seconds {script | procname ?args?}");

    if (Tcl_GetInt(irp, argv[1], &seconds) != TCL_OK) {
        return TCL_ERROR;
    }

    if ((timerid = insert_timer(seconds, argv[2])) == -1) {
        Tcl_AppendResult(irp, "0", NULL);
        return TCL_ERROR;
    }

    sprintf(reply, "%d", timerid);

    Tcl_AppendResult(irp, reply, NULL);

    queue_nexttimer();

    return TCL_OK;
}

static int tcl_on(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    int newentry;
    char *procList;
    Tcl_HashEntry *entry;
    char *token;
    int dummy;
    Tcl_DString ds;
    int index;
    int count;
    int list_argc, proc_argc;
    char **list_argv, **proc_argv;

    BADARGS(4, 4, " token label {script | procname ?args?}");

    token = StrDup(argv[1], &dummy);
    Tcl_UtfToUpper(token);

    Tcl_DStringInit(&ds);

    entry = Tcl_CreateHashEntry(&cmdTablePtr, token, &newentry);
    if (!newentry) {
        procList = Tcl_GetHashValue(entry);
        if (Tcl_SplitList(interp, procList, &list_argc, &list_argv) == TCL_OK) {
            for (count = 0; count < list_argc; count++) {
                if (Tcl_SplitList(interp, list_argv[count], &proc_argc, &proc_argv) != TCL_OK)
                    continue;
                if (strcmp(proc_argv[0], argv[2])) {
                    Tcl_DStringStartSublist(&ds);
                    Tcl_DStringAppendElement(&ds, proc_argv[0]);
                    Tcl_DStringAppendElement(&ds, proc_argv[1]);
                    Tcl_DStringEndSublist(&ds);
                }
                Tcl_Free((char *) proc_argv);
            }
            Tcl_Free((char *) list_argv);
        }
        Tcl_Free(procList);
    }

    Tcl_DStringStartSublist(&ds);
    Tcl_DStringAppendElement(&ds, argv[2]);
    Tcl_DStringAppendElement(&ds, argv[3]);
    Tcl_DStringEndSublist(&ds);

    procList = StrDup(ds.string, &dummy);

    Tcl_SetHashValue(entry, procList);

    if ((strncmp(token, "XC_", 3) == 0) || (strcmp(token, "CHAT") == 0)) {
        for (index = 0; index < XC_SIZE; index++) {
            if (strcmp(xc[index].event, token) == 0) {
                if (xc[index].hook == NULL) {
                    xc[index].hook = xchat_hook_print(ph, xc[index].emit, XCHAT_PRI_NORM, Print_Hook, (void *) index);
                    break;
                }
            }
        }
    }

    Tcl_Free(token);
    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_off(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    char *procList;
    Tcl_HashEntry *entry;
    char *token;
    int dummy;
    Tcl_DString ds;
    int index;
    int count;
    int list_argc, proc_argc;
    char **list_argv, **proc_argv;

    BADARGS(2, 3, " token ?label?");

    token = StrDup(argv[1], &dummy);
    Tcl_UtfToUpper(token);

    Tcl_DStringInit(&ds);

    if ((entry = Tcl_FindHashEntry(&cmdTablePtr, token)) != NULL) {

        procList = Tcl_GetHashValue(entry);

        if (argc == 3) {
            if (Tcl_SplitList(interp, procList, &list_argc, &list_argv) == TCL_OK) {
                for (count = 0; count < list_argc; count++) {
                    if (Tcl_SplitList(interp, list_argv[count], &proc_argc, &proc_argv) != TCL_OK)
                        continue;
                    if (strcmp(proc_argv[0], argv[2])) {
                        Tcl_DStringStartSublist(&ds);
                        Tcl_DStringAppendElement(&ds, proc_argv[0]);
                        Tcl_DStringAppendElement(&ds, proc_argv[1]);
                        Tcl_DStringEndSublist(&ds);
                    }
                    Tcl_Free((char *) proc_argv);
                }
                Tcl_Free((char *) list_argv);
            }
        }

        Tcl_Free(procList);

        if (ds.length) {
            procList = StrDup(ds.string, &dummy);
            Tcl_SetHashValue(entry, procList);
        } else {
            Tcl_DeleteHashEntry(entry);
        }

        if (!ds.length) {
            if ((strncmp(token, "XC_", 3) == 0) || (strcmp(token, "CHAT") == 0)) {
                for (index = 0; index < XC_SIZE; index++) {
                    if (strcmp(xc[index].event, token) == 0) {
                        if (xc[index].hook != NULL) {
                            xchat_unhook(ph, xc[index].hook);
                            break;
                        }
                    }
                }
            }
        }
    }

    Tcl_Free(token);
    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_alias(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    int newentry;
    int proclen;
    alias *aliasPtr;
    Tcl_HashEntry *entry;
    char *string;
    int dummy;

    BADARGS(3, 3, " name {script | procname ?args?}");

    string = StrDup(argv[1], &dummy);
    Tcl_UtfToUpper(string);

    proclen = strlen(argv[2]);

    entry = Tcl_CreateHashEntry(&aliasTablePtr, string, &newentry);
    if (newentry) {
        aliasPtr = (alias *) Tcl_Alloc(sizeof(alias));
        aliasPtr->hook = xchat_hook_command(ph, string, XCHAT_PRI_NORM, Command_Alias, 0, 0);
    } else {
        aliasPtr = Tcl_GetHashValue(entry);
        Tcl_Free(aliasPtr->procPtr);
    }

    aliasPtr->procPtr = StrDup(argv[2], &dummy);

    Tcl_SetHashValue(entry, aliasPtr);

    Tcl_Free(string);

    return TCL_OK;
}


static int tcl_complete(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    BADARGS(1, 2, " ?EAT_NONE|EAT_XCHAT|EAT_PLUGIN|EAT_ALL?");

    if (argc == 2) {
        if (Tcl_GetInt(irp, argv[1], &complete) != TCL_OK) {
            if (strcasecmp("EAT_NONE", argv[1]) == 0)
                complete = XCHAT_EAT_NONE;
            else if (strcasecmp("EAT_XCHAT", argv[1]) == 0)
                complete = XCHAT_EAT_XCHAT;
            else if (strcasecmp("EAT_PLUGIN", argv[1]) == 0)
                complete = XCHAT_EAT_PLUGIN;
            else if (strcasecmp("EAT_ALL", argv[1]) == 0)
                complete = XCHAT_EAT_ALL;
            else
                BADARGS(1, 2, " ?EAT_NONE|EAT_XCHAT|EAT_PLUGIN|EAT_ALL?");
        }
    } else
        complete = XCHAT_EAT_PLUGIN;

    return TCL_RETURN;
}


static int tcl_command(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx;
    xchat_context *ctx = NULL;
    char *string = NULL;

    BADARGS(2, 4, " ?server? ?#channel|nick? text");

    origctx = xchat_get_context(ph);

    switch (argc) {
    case 2:
        ctx = origctx;
        break;
    case 3:
        ctx = xchat_smart_context(argv[1], NULL);
        break;
    case 4:
        ctx = xchat_smart_context(argv[1], argv[2]);
        break;
    default:;
    }

    CHECKCTX(ctx);

    string = argv[argc - 1];

    if (string[0] == '/')
        string++;

    xchat_set_context(ph, ctx);
    xchat_command(ph, string);
    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_raw(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx;
    xchat_context *ctx = NULL;
    char *string = NULL;

    BADARGS(2, 4, " ?server? ?#channel|nick? text");

    origctx = xchat_get_context(ph);

    switch (argc) {
    case 2:
        ctx = origctx;
        break;
    case 3:
        ctx = xchat_smart_context(argv[1], NULL);
        break;
    case 4:
        ctx = xchat_smart_context(argv[1], argv[2]);
        break;
    default:;
    }

    CHECKCTX(ctx);

    string = argv[argc - 1];

    xchat_set_context(ph, ctx);
    xchat_commandf(ph, "RAW %s", string);
    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_getinfo(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    BADARGS(2, 2, " id");
    Tcl_AppendResult(irp, xchat_get_info(ph, argv[1]), NULL);
    return TCL_OK;
}

static int tcl_server(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    BADARGS(1, 1, "");
    Tcl_AppendResult(irp, xchat_get_info(ph, "server"), NULL);
    return TCL_OK;
}

static int tcl_channel(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    BADARGS(1, 1, "");
    Tcl_AppendResult(irp, xchat_get_info(ph, "channel"), NULL);
    return TCL_OK;
}

static int tcl_version(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    BADARGS(1, 1, "");
    Tcl_AppendResult(irp, xchat_get_info(ph, "version"), NULL);
    return TCL_OK;
}

static int tcl_xchatdir(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    BADARGS(1, 1, "");
    Tcl_AppendResult(irp, xchat_get_info(ph, "xchatdir"), NULL);
    return TCL_OK;
}

static int tcl_getlist(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_list *list = NULL;
    const char *name = NULL;
    const char **fields;
    const char *field;
    const char *sattr;
    int iattr;
    int i;
    Tcl_DString ds;
    xchat_context *ctx;

    BADARGS(1, 2, " list");

    Tcl_DStringInit(&ds);

    fields = xchat_list_fields(ph, "lists");

    if (argc == 1) {
        for (i = 0; fields[i] != NULL; i++) {
            Tcl_DStringAppendElement(&ds, fields[i]);
        }
        goto done;
    }

    for (i = 0; fields[i] != NULL; i++) {
        if (strcmp(fields[i], argv[1]) == 0) {
            name = fields[i];
            break;
        }
    }

    if (name == NULL)
        goto done;

    list = xchat_list_get(ph, name);
    if (list == NULL)
        goto done;

    fields = xchat_list_fields(ph, name);

    Tcl_DStringStartSublist(&ds);
    for (i = 0; fields[i] != NULL; i++) {
        field = fields[i] + 1;
        Tcl_DStringAppendElement(&ds, field);
    }
    Tcl_DStringEndSublist(&ds);

    while (xchat_list_next(ph, list)) {

        Tcl_DStringStartSublist(&ds);

        for (i = 0; fields[i] != NULL; i++) {

            field = fields[i] + 1;

            switch (fields[i][0]) {
            case 's':
                sattr = xchat_list_str(ph, list, (char *) field);
                Tcl_DStringAppendElement(&ds, sattr);
                break;
            case 'i':
                iattr = xchat_list_int(ph, list, (char *) field);
                Tcl_DStringAppendElement(&ds, myitoa(iattr));
                break;
            case 'p':
                sattr = xchat_list_str(ph, list, (char *) field);
                if (strcmp(field, "context") == 0) {
                    ctx = (xchat_context *) sattr;
                    Tcl_DStringStartSublist(&ds);
                    Tcl_DStringAppendElement(&ds, "ctx");
                    Tcl_DStringAppendElement(&ds, xchat_get_info(ph, "server"));
                    Tcl_DStringAppendElement(&ds, xchat_get_info(ph, "channel"));
                    Tcl_DStringEndSublist(&ds);
                } else
                    Tcl_DStringAppendElement(&ds, "*");
                break;
            default:
                Tcl_DStringAppendElement(&ds, "*");
                break;
            }
        }

        Tcl_DStringEndSublist(&ds);

    }

    xchat_list_free(ph, list);

  done:

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}

/*
 * tcl_xchat_puts - stub for tcl puts command
 * This is modified from the original internal "puts" command.  It redirects
 * stdout to the current context, while still allowing all normal puts features
 */

static int tcl_xchat_puts(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    Tcl_Channel chan;
    char *string;
    int newline;
    char *channelId = NULL;
    int result;
    int mode;
    int trap_stdout = 0;

    switch (argc) {

    case 2:
        string = argv[1];
        newline = 1;
        trap_stdout = 1;
        break;

    case 3:
        if (strcmp(argv[1], "-nonewline") == 0) {
            newline = 0;
            trap_stdout = 1;
        } else {
            newline = 1;
            channelId = argv[1];
        }
        string = argv[2];
        break;

    case 4:
        if (strcmp(argv[1], "-nonewline") == 0) {
            channelId = argv[2];
            string = argv[3];
        } else {
            if (strcmp(argv[3], "nonewline") != 0) {
                Tcl_AppendResult(interp, "bad argument \"", argv[3], "\": should be \"nonewline\"", (char *) NULL);
                return TCL_ERROR;
            }
            channelId = argv[1];
            string = argv[2];
        }
        newline = 0;
        break;

    default:
        Tcl_AppendResult(interp, argv, "?-nonewline? ?channelId? string", NULL);
        return TCL_ERROR;
    }

    if (!trap_stdout && (strcmp(channelId, "stdout") == 0))
        trap_stdout = 1;

    if (trap_stdout) {
        if (newline)
            xchat_printf(ph, "%s\n", string);
        else
            xchat_print(ph, string);
        return TCL_OK;
    }

    chan = Tcl_GetChannel(interp, channelId, &mode);
    if (chan == (Tcl_Channel) NULL) {
        return TCL_ERROR;
    }
    if ((mode & TCL_WRITABLE) == 0) {
        Tcl_AppendResult(interp, "channel \"", channelId, "\" wasn't opened for writing", (char *) NULL);
        return TCL_ERROR;
    }

    result = Tcl_Write(chan, string, strlen(string));
    if (result < 0) {
        goto error;
    }
    if (newline != 0) {
        result = Tcl_WriteChars(chan, "\n", 1);
        if (result < 0) {
            goto error;
        }
    }
    return TCL_OK;

  error:
    Tcl_AppendResult(interp, "error writing \"", channelId, "\": ", Tcl_PosixError(interp), (char *) NULL);

    return TCL_ERROR;
}

static int tcl_print(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx;
    xchat_context *ctx = NULL;
    char *string = NULL;

    BADARGS(2, 4, " ?server? ?#channel|nick? text");

    origctx = xchat_get_context(ph);

    switch (argc) {
    case 2:
        ctx = origctx;
        break;
    case 3:
        ctx = xchat_smart_context(argv[1], NULL);
        break;
    case 4:
        ctx = xchat_smart_context(argv[1], argv[2]);
        break;
    default:;
    }

    CHECKCTX(ctx);

    string = argv[argc - 1];

    xchat_set_context(ph, ctx);
    xchat_print(ph, string);
    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_setcontext(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *ctx = NULL;
    int list_argc;
    char **list_argv;

    BADARGS(2, 2, " context");

    if (Tcl_SplitList(interp, argv[1], &list_argc, &list_argv) == TCL_OK) {
        if ((argc != 3) || strcmp(list_argv[0], "ctx")) {
            Tcl_AppendResult(irp, "Invalid context", NULL);
            return TCL_ERROR;
        }
        ctx = xchat_smart_context(list_argv[1], list_argv[2]);
    }

    CHECKCTX(ctx);

    xchat_set_context(ph, ctx);

    return TCL_OK;
}

static int tcl_findcontext(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx, *ctx = NULL;
    Tcl_DString ds;

    BADARGS(2, 3, " ?server? ?channel?");

    switch (argc) {
    case 2:
        ctx = xchat_smart_context(argv[1], NULL);
        break;
    case 3:
        ctx = xchat_smart_context(argv[1], argv[2]);
        break;
    default:;
    }

    Tcl_DStringInit(&ds);

    origctx = xchat_get_context(ph);
    xchat_set_context(ph, ctx);
    Tcl_DStringAppendElement(&ds, "ctx");
    Tcl_DStringAppendElement(&ds, xchat_get_info(ph, "server"));
    Tcl_DStringAppendElement(&ds, xchat_get_info(ph, "channel"));
    xchat_set_context(ph, origctx);

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}


static int tcl_getcontext(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *ctx = NULL;
    Tcl_DString ds;

    BADARGS(1, 1, "");

    Tcl_DStringInit(&ds);

    ctx = xchat_get_context(ph);

    Tcl_DStringAppendElement(&ds, "ctx");
    Tcl_DStringAppendElement(&ds, xchat_get_info(ph, "server"));
    Tcl_DStringAppendElement(&ds, xchat_get_info(ph, "channel"));

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_channels(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    char *server, *channel;
    xchat_list *list;
    Tcl_DString ds;

    BADARGS(1, 2, " ?server?");

    if (argc == 1)
        server = (char *) xchat_get_info(ph, "server");
    else
        server = argv[1];

    Tcl_DStringInit(&ds);

    list = xchat_list_get(ph, "channels");

    if (list != NULL) {
        while (xchat_list_next(ph, list)) {
            if (strcmp(server, xchat_list_str(ph, list, "server")) != 0)
                continue;
            (const char *) channel = xchat_list_str(ph, list, "channel");
            if ((channel[0] == '#') || (channel[0] == '&'))
                Tcl_DStringAppendElement(&ds, channel);
        }
        xchat_list_free(ph, list);
    }

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_servers(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    char *server, *channel;
    xchat_list *list;
    Tcl_DString ds;

    BADARGS(1, 1, "");

    Tcl_DStringInit(&ds);

    list = xchat_list_get(ph, "channels");

    if (list != NULL) {
        while (xchat_list_next(ph, list)) {
            (const char *) channel = xchat_list_str(ph, list, "channel");
            (const char *) server = xchat_list_str(ph, list, "server");
            if (strcmp(channel, server) == 0)
                Tcl_DStringAppendElement(&ds, server);
        }
        xchat_list_free(ph, list);
    }

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_queries(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    char *server, *channel;
    xchat_list *list;
    Tcl_DString ds;

    BADARGS(1, 2, " ?server?");

    if (argc == 1)
        server = (char *) xchat_get_info(ph, "server");
    else
        server = argv[1];

    Tcl_DStringInit(&ds);

    list = xchat_list_get(ph, "channels");

    if (list != NULL) {
        while (xchat_list_next(ph, list)) {
            if (strcmp(server, xchat_list_str(ph, list, "server")) != 0)
                continue;
            (const char *) channel = xchat_list_str(ph, list, "channel");
            if (strcmp(channel, server) == 0)
                continue;
            if ((channel[0] != '#') && (channel[0] != '&'))
                Tcl_DStringAppendElement(&ds, channel);
        }
        xchat_list_free(ph, list);
    }

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_users(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx, *ctx = NULL;
    xchat_list *list;
    Tcl_DString ds;

    BADARGS(1, 3, " ?server? ?channel?");

    origctx = xchat_get_context(ph);

    switch (argc) {
    case 1:
        ctx = origctx;
        break;
    case 2:
        ctx = xchat_smart_context(argv[1], NULL);
        break;
    case 3:
        ctx = xchat_smart_context(argv[1], argv[2]);
        break;
    default:;
    }

    CHECKCTX(ctx);

    xchat_set_context(ph, ctx);

    Tcl_DStringInit(&ds);

    list = xchat_list_get(ph, "users");

    if (list != NULL) {

        Tcl_DStringStartSublist(&ds);
        Tcl_DStringAppendElement(&ds, "nick");
        Tcl_DStringAppendElement(&ds, "host");
        Tcl_DStringAppendElement(&ds, "prefix");
        Tcl_DStringEndSublist(&ds);

        while (xchat_list_next(ph, list)) {;
            Tcl_DStringStartSublist(&ds);
            Tcl_DStringAppendElement(&ds, (const char *) xchat_list_str(ph, list, "nick"));
            Tcl_DStringAppendElement(&ds, (const char *) xchat_list_str(ph, list, "host"));
            Tcl_DStringAppendElement(&ds, (const char *) xchat_list_str(ph, list, "prefix"));
            Tcl_DStringEndSublist(&ds);
        }

        xchat_list_free(ph, list);
    }

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_chats(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_list *list;
    Tcl_DString ds;

    BADARGS(1, 1, "");

    Tcl_DStringInit(&ds);

    list = xchat_list_get(ph, "dcc");

    if (list != NULL) {
        while (xchat_list_next(ph, list)) {;
            switch (xchat_list_int(ph, list, "type")) {
            case 2:
            case 3:
                if (xchat_list_int(ph, list, "status") == 1)
                    Tcl_DStringAppendElement(&ds, (const char *) xchat_list_str(ph, list, "nick"));
                break;
            }
        }
        xchat_list_free(ph, list);
    }

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_ignores(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_list *list;
    Tcl_DString ds;
    int flags;

    BADARGS(1, 1, "");

    Tcl_DStringInit(&ds);

    list = xchat_list_get(ph, "ignore");

    if (list != NULL) {

        while (xchat_list_next(ph, list)) {;
            Tcl_DStringStartSublist(&ds);
            Tcl_DStringAppendElement(&ds, (const char *) xchat_list_str(ph, list, "mask"));
            Tcl_DStringStartSublist(&ds);
            flags = xchat_list_int(ph, list, "flags");
            if (flags & 1)
                Tcl_DStringAppendElement(&ds, "PRIVMSG");
            if (flags & 2)
                Tcl_DStringAppendElement(&ds, "NOTICE");
            if (flags & 4)
                Tcl_DStringAppendElement(&ds, "CHANNEL");
            if (flags & 8)
                Tcl_DStringAppendElement(&ds, "CTCP");
            if (flags & 16)
                Tcl_DStringAppendElement(&ds, "INVITE");
            if (flags & 32)
                Tcl_DStringAppendElement(&ds, "UNIGNORE");
            if (flags & 64)
                Tcl_DStringAppendElement(&ds, "NOSAVE");
            Tcl_DStringEndSublist(&ds);
            Tcl_DStringEndSublist(&ds);
        }
        xchat_list_free(ph, list);
    }

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_dcclist(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_list *list;
    Tcl_DString ds;
    int dcctype;

    BADARGS(1, 1, "");

    Tcl_DStringInit(&ds);

    list = xchat_list_get(ph, "dcc");

    if (list != NULL) {

        while (xchat_list_next(ph, list)) {;
            Tcl_DStringStartSublist(&ds);
            dcctype = xchat_list_int(ph, list, "type");
            switch (dcctype) {
            case 0:
                Tcl_DStringAppendElement(&ds, "filesend");
                break;
            case 1:
                Tcl_DStringAppendElement(&ds, "filerecv");
                break;
            case 2:
                Tcl_DStringAppendElement(&ds, "chatrecv");
                break;
            case 3:
                Tcl_DStringAppendElement(&ds, "chatsend");
                break;
            }
            switch (xchat_list_int(ph, list, "status")) {
            case 0:
                Tcl_DStringAppendElement(&ds, "queued");
                break;
            case 1:
                Tcl_DStringAppendElement(&ds, "active");
                break;
            case 2:
                Tcl_DStringAppendElement(&ds, "failed");
                break;
            case 3:
                Tcl_DStringAppendElement(&ds, "done");
                break;
            case 4:
                Tcl_DStringAppendElement(&ds, "connecting");
                break;
            case 5:
                Tcl_DStringAppendElement(&ds, "aborted");
                break;
            }

            Tcl_DStringAppendElement(&ds, (const char *) xchat_list_str(ph, list, "nick"));

            switch (dcctype) {
            case 0:
                Tcl_DStringAppendElement(&ds, (const char *) xchat_list_str(ph, list, "file"));
                break;
            case 1:
                Tcl_DStringAppendElement(&ds, (const char *) xchat_list_str(ph, list, "destfile"));
                break;
            }
            Tcl_DStringAppendElement(&ds, myitoa(xchat_list_int(ph, list, "size")));
            Tcl_DStringAppendElement(&ds, myitoa(xchat_list_int(ph, list, "resume")));
            Tcl_DStringAppendElement(&ds, myitoa(xchat_list_int(ph, list, "pos")));
            Tcl_DStringAppendElement(&ds, myitoa(xchat_list_int(ph, list, "cps")));
            Tcl_DStringEndSublist(&ds);
        }
        xchat_list_free(ph, list);
    }

    Tcl_AppendResult(irp, ds.string, NULL);

    Tcl_DStringFree(&ds);

    return TCL_OK;
}

static int tcl_away(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx, *ctx;
    BADARGS(1, 2, " ?server?");

    origctx = xchat_get_context(ph);

    if (argc == 2) {
        ctx = xchat_find_context(ph, argv[1], NULL);
        CHECKCTX(ctx);
        xchat_set_context(ph, ctx);
    }

    Tcl_AppendResult(irp, xchat_get_info(ph, "away"), NULL);

    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_host(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx, *ctx;
    BADARGS(1, 2, " ?server?");

    origctx = xchat_get_context(ph);

    if (argc == 2) {
        ctx = xchat_find_context(ph, argv[1], NULL);
        CHECKCTX(ctx);
        xchat_set_context(ph, ctx);
    }

    Tcl_AppendResult(irp, xchat_get_info(ph, "host"), NULL);

    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_network(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx, *ctx;
    BADARGS(1, 2, " ?server?");

    origctx = xchat_get_context(ph);

    if (argc == 2) {
        ctx = xchat_find_context(ph, argv[1], NULL);
        CHECKCTX(ctx);
        xchat_set_context(ph, ctx);
    }

    Tcl_AppendResult(irp, xchat_get_info(ph, "network"), NULL);

    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_me(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx, *ctx;
    BADARGS(1, 2, " ?server?");

    origctx = xchat_get_context(ph);

    if (argc == 2) {
        ctx = xchat_find_context(ph, argv[1], NULL);
        CHECKCTX(ctx);
        xchat_set_context(ph, ctx);
    }

    Tcl_AppendResult(irp, xchat_get_info(ph, "nick"), NULL);

    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_topic(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    xchat_context *origctx, *ctx = NULL;
    BADARGS(1, 3, " ?server? ?channel?");

    origctx = xchat_get_context(ph);

    switch (argc) {
    case 1:
        ctx = origctx;
        break;
    case 2:
        ctx = xchat_smart_context(argv[1], NULL);
        break;
    case 3:
        ctx = xchat_smart_context(argv[1], argv[2]);
        break;
    default:;
    }

    CHECKCTX(ctx);

    xchat_set_context(ph, ctx);
    Tcl_AppendResult(irp, xchat_get_info(ph, "topic"), NULL);
    xchat_set_context(ph, origctx);

    return TCL_OK;
}

static int tcl_xchat_nickcmp(ClientData cd, Tcl_Interp * irp, int argc, char *argv[])
{
    BADARGS(3, 3, " string1 string2");
    Tcl_AppendResult(irp, myitoa(xchat_nickcmp(ph, argv[1], argv[2])), NULL);
    return TCL_OK;
}

static int Command_Alias(char *word[], char *word_eol[], void *userdata)
{
    alias *aliasPtr;
    Tcl_HashEntry *entry;
    xchat_context *origctx;
    int dummy;
    char *string;

    complete = XCHAT_EAT_NONE;

    string = StrDup(word[1], &dummy);
    Tcl_UtfToUpper(string);

    if ((entry = Tcl_FindHashEntry(&aliasTablePtr, string)) != NULL) {
        aliasPtr = Tcl_GetHashValue(entry);
        origctx = xchat_get_context(ph);
        Tcl_SetVar(interp, "_cmd", word[1], TCL_NAMESPACE_ONLY);
        Tcl_SetVar(interp, "_rest", word_eol[2], TCL_NAMESPACE_ONLY);
        if (Tcl_Eval(interp, aliasPtr->procPtr) == TCL_ERROR) {
            xchat_printf(ph, "\0039Tcl plugin:\003\tERROR (%s) %s \n", string, Tcl_GetStringResult(interp));
        }
        xchat_set_context(ph, origctx);
        complete = XCHAT_EAT_ALL;
    }

    Tcl_Free(string);

    return complete;            /* see 'tcl_complete */
}

static int Command_TCL(char *word[], char *word_eol[], void *userdata)
{
    if (Tcl_Eval(interp, word_eol[2]) == TCL_ERROR)
        xchat_printf(ph, "\0039Tcl plugin\003\tERROR: %s\n", Tcl_GetStringResult(interp));
    else
        xchat_printf(ph, "\0039Tcl plugin\003\tRESULT: %s\n", Tcl_GetStringResult(interp));
    return XCHAT_EAT_ALL;
}

static int Command_Source(char *word[], char *word_eol[], void *userdata)
{
    char *xchatdir;
    Tcl_DString ds;
    struct stat dummy;
    int len;

    if (!strlen(word_eol[2]))
        return XCHAT_EAT_NONE;

    len = strlen(word[2]);

    if (len > 4 && strcasecmp(".tcl", word[2] + len - 4) == 0) {

        (const char *) xchatdir = xchat_get_info(ph, "xchatdir");

        Tcl_DStringInit(&ds);

        if (stat(word_eol[2], &dummy) == 0) {
            Tcl_DStringAppend(&ds, word_eol[2], strlen(word_eol[2]));
        } else {
            if (!strchr(word_eol[2], '/')) {
                Tcl_DStringAppend(&ds, xchatdir, strlen(xchatdir));
                Tcl_DStringAppend(&ds, "/", 1);
                Tcl_DStringAppend(&ds, word_eol[2], strlen(word_eol[2]));
            }
        }

        if (Tcl_EvalFile(interp, ds.string) == TCL_ERROR)
            xchat_printf(ph, "\0039Tcl plugin:\003\tERROR: %s\n", Tcl_GetStringResult(interp));
        else
            xchat_printf(ph, "\0039Tcl plugin:\003\tSourced %s\n", ds.string);

        Tcl_DStringFree(&ds);

        if (strcasecmp("LOAD", word[1]) == 0)
            return XCHAT_EAT_XCHAT;
        else
            return XCHAT_EAT_ALL;

    } else
        return XCHAT_EAT_NONE;

}

static int Command_Rehash(char *word[], char *word_eol[], void *userdata)
{
    Tcl_Plugin_DeInit();
    Tcl_Plugin_Init();
    xchat_print(ph, "\0039Tcl plugin:\003\tRehashed\n");
    return XCHAT_EAT_ALL;
}

static int TCL_Event_Handler(void *userdata)
{
    Tcl_DoOneEvent(TCL_DONT_WAIT);
    do_timer();
    return 1;
}

static void Tcl_Plugin_Init()
{
    int x;
    char *xchatdir;

    interp = Tcl_CreateInterp();
    Tcl_Init(interp);

    Tcl_CreateCommand(interp, "alias", tcl_alias, NULL, NULL);
    Tcl_CreateCommand(interp, "away", tcl_away, NULL, NULL);
    Tcl_CreateCommand(interp, "channel", tcl_channel, NULL, NULL);
    Tcl_CreateCommand(interp, "channels", tcl_channels, NULL, NULL);
    Tcl_CreateCommand(interp, "chats", tcl_chats, NULL, NULL);
    Tcl_CreateCommand(interp, "command", tcl_command, NULL, NULL);
    Tcl_CreateCommand(interp, "complete", tcl_complete, NULL, NULL);
    Tcl_CreateCommand(interp, "dcclist", tcl_dcclist, NULL, NULL);
    Tcl_CreateCommand(interp, "findcontext", tcl_findcontext, NULL, NULL);
    Tcl_CreateCommand(interp, "getcontext", tcl_getcontext, NULL, NULL);
    Tcl_CreateCommand(interp, "getinfo", tcl_getinfo, NULL, NULL);
    Tcl_CreateCommand(interp, "getlist", tcl_getlist, NULL, NULL);
    Tcl_CreateCommand(interp, "host", tcl_host, NULL, NULL);
    Tcl_CreateCommand(interp, "ignores", tcl_ignores, NULL, NULL);
    Tcl_CreateCommand(interp, "killtimer", tcl_killtimer, NULL, NULL);
    Tcl_CreateCommand(interp, "me", tcl_me, NULL, NULL);
    Tcl_CreateCommand(interp, "network", tcl_network, NULL, NULL);
    Tcl_CreateCommand(interp, "on", tcl_on, NULL, NULL);
    Tcl_CreateCommand(interp, "off", tcl_off, NULL, NULL);
    Tcl_CreateCommand(interp, "nickcmp", tcl_xchat_nickcmp, NULL, NULL);
    Tcl_CreateCommand(interp, "print", tcl_print, NULL, NULL);
    Tcl_CreateCommand(interp, "::puts", tcl_xchat_puts, NULL, NULL);
    Tcl_CreateCommand(interp, "queries", tcl_queries, NULL, NULL);
    Tcl_CreateCommand(interp, "raw", tcl_raw, NULL, NULL);
    Tcl_CreateCommand(interp, "server", tcl_server, NULL, NULL);
    Tcl_CreateCommand(interp, "servers", tcl_servers, NULL, NULL);
    Tcl_CreateCommand(interp, "setcontext", tcl_setcontext, NULL, NULL);
    Tcl_CreateCommand(interp, "timer", tcl_timer, NULL, NULL);
    Tcl_CreateCommand(interp, "timerexists", tcl_timerexists, NULL, NULL);
    Tcl_CreateCommand(interp, "timers", tcl_timers, NULL, NULL);
    Tcl_CreateCommand(interp, "topic", tcl_topic, NULL, NULL);
    Tcl_CreateCommand(interp, "users", tcl_users, NULL, NULL);
    Tcl_CreateCommand(interp, "version", tcl_version, NULL, NULL);
    Tcl_CreateCommand(interp, "xchatdir", tcl_xchatdir, NULL, NULL);

    Tcl_InitHashTable(&cmdTablePtr, TCL_STRING_KEYS);
    Tcl_InitHashTable(&aliasTablePtr, TCL_STRING_KEYS);

    for (x = 0; x < MAX_TIMERS; x++) {
        timers[x].timerid = 0;
        timers[x].procPtr = NULL;
    }

    for (x = 0; x < XC_SIZE; x++)
        xc[x].hook = NULL;

    if (Tcl_Eval(interp, unknown) == TCL_ERROR) {
        xchat_printf(ph, "Error sourcing internal 'unknown' (%s)\n", Tcl_GetStringResult(interp));
    }

    (const char *) xchatdir = xchat_get_info(ph, "xchatdir");

    if (Tcl_Eval(interp, sourcedirs) == TCL_ERROR) {
        xchat_printf(ph, "Error sourcing internal 'sourcedirs' (%s)\n", Tcl_GetStringResult(interp));
    }

}

static void Tcl_Plugin_DeInit()
{
    int x;
    char *procPtr;
    alias *aliasPtr;
    Tcl_HashEntry *entry;
    Tcl_HashSearch search;

    /* Be sure to free all the memory for ON and ALIAS entries */

    entry = Tcl_FirstHashEntry(&cmdTablePtr, &search);
    while (entry != NULL) {
        procPtr = Tcl_GetHashValue(entry);
        Tcl_Free(procPtr);
        entry = Tcl_NextHashEntry(&search);
    }

    Tcl_DeleteHashTable(&cmdTablePtr);

    entry = Tcl_FirstHashEntry(&aliasTablePtr, &search);
    while (entry != NULL) {
        aliasPtr = Tcl_GetHashValue(entry);
        Tcl_Free(aliasPtr->procPtr);
        xchat_unhook(ph, aliasPtr->hook);
        Tcl_Free((char *) aliasPtr);
        entry = Tcl_NextHashEntry(&search);
    }

    Tcl_DeleteHashTable(&aliasTablePtr);

    for (x = 1; x < MAX_TIMERS; x++) {
        if (timers[x].timerid) {
            timers[x].timerid = 0;
            if (timers[x].procPtr != NULL)
                Tcl_Free(timers[x].procPtr);
            timers[x].procPtr = NULL;
            break;
        }
    }

    for (x = 0; x < XC_SIZE; x++) {
        if (xc[x].hook != NULL) {
            xchat_unhook(ph, xc[x].hook);
            xc[x].hook = NULL;
        }
    }

    Tcl_DeleteInterp(interp);
}

static void banner()
{
    xchat_printf(ph, "Tcl plugin for XChat - Version %s\n", VERSION);
    xchat_print(ph, "Copyright 2002-2003 Daniel P. Stasinski\n");
    xchat_print(ph, "http://www.scriptkitties.com/tclplugin/\n");
}

int xchat_plugin_init(xchat_plugin * plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
    if (initialized != 0) {
        banner();
        xchat_print(ph, "Tcl plugin already loaded.");
        reinit_tried++;
        return 0;
    }
    initialized = 1;

    ph = plugin_handle;

    *plugin_name = "Tcl plugin";
    *plugin_desc = "TCL scripting interface";
    *plugin_version = VERSION;

    Tcl_Plugin_Init();

    raw_line_hook = xchat_hook_server(ph, "RAW LINE", XCHAT_PRI_NORM, Server_raw_line, NULL);
    Command_TCL_hook = xchat_hook_command(ph, "tcl", XCHAT_PRI_NORM, Command_TCL, 0, 0);
    Command_Source_hook = xchat_hook_command(ph, "source", XCHAT_PRI_NORM, Command_Source, 0, 0);
    Command_Rehash_hook = xchat_hook_command(ph, "rehash", XCHAT_PRI_NORM, Command_Rehash, 0, 0);
    Command_Load_hook = xchat_hook_command(ph, "LOAD", XCHAT_PRI_NORM, Command_Source, 0, 0);
    Event_Handler_hook = xchat_hook_timer(ph, 100, TCL_Event_Handler, 0);

    banner();
    xchat_print(ph, "Tcl plugin loaded successfully!\n");

    return 1;                   /* return 1 for success */
}

int xchat_plugin_deinit()
{
    if (reinit_tried) {
        reinit_tried--;
        return 1;
    }

    xchat_unhook(ph, raw_line_hook);
    xchat_unhook(ph, Command_TCL_hook);
    xchat_unhook(ph, Command_Source_hook);
    xchat_unhook(ph, Command_Rehash_hook);
    xchat_unhook(ph, Command_Load_hook);
    xchat_unhook(ph, Event_Handler_hook);

    Tcl_Plugin_DeInit();

    xchat_print(ph, "Tcl plugin unloaded.\n");
    initialized = 0;

    return 1;
}
