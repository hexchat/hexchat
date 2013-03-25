#ifndef BANLIST_H
#define BANLIST_H

#include "../common/hexchat.h"
void banlist_opengui (session *sess);

#ifndef RPL_BANLIST
/* Where's that darn header file, that would have all these defines ? */
#define RPL_BANLIST 367
#define RPL_ENDOFBANLIST 368
#define RPL_INVITELIST 346
#define RPL_ENDOFINVITELIST 347
#define RPL_EXCEPTLIST 348
#define RPL_ENDOFEXCEPTLIST 349
#define RPL_QUIETLIST 728
#define RPL_ENDOFQUIETLIST 729
#endif

typedef enum banlist_modes_e {
	MODE_BAN,
	MODE_EXEMPT,
	MODE_INVITE,
	MODE_QUIET,
	MODE_CT
} banlist_modes;

typedef struct banlist_info_s banlist_info;

typedef struct mode_info_s {
	char *name;		/* Checkbox name, e.g. "Bans" */
	char *type;		/* Type for type column, e.g. "Ban" */
	char letter;	/* /mode-command letter, e.g. 'b' for MODE_BAN */
	int code;		/* rfc RPL_foo code, e.g. 367 for RPL_BANLIST */
	int endcode;	/* rfc RPL_ENDOFfoo code, e.g. 368 for RPL_ENDOFBANLIST */
	int bit;			/* Mask bit, e.g., 1<<MODE_BAN  */
	void (*tester)(banlist_info *, int);	/* Function returns true to set bit into checkable */
} mode_info;

typedef struct banlist_info_s {
	session *sess;
	int capable;	/* MODE bitmask */
	int readable;	/* subset of capable if not op */
	int writeable;	/* subset of capable if op */
	int checked;	/* subset of (op? writeable: readable) */
	int pending;	/* subset of checked */
	int current;	/* index of currently processing mode */
	int line_ct;	/* count of presented lines */
	int select_ct;	/* count of selected lines */
		/* Not really; 1 if any are selected otherwise 0 */
	GtkWidget *window;
	GtkWidget *treeview;
	GtkWidget *checkboxes[MODE_CT];
	GtkWidget *but_remove;
	GtkWidget *but_crop;
	GtkWidget *but_clear;
	GtkWidget *but_refresh;
} banlist_info;
#endif /* BANLIST_H */
