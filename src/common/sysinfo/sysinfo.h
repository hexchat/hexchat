#ifndef HEXCHAT_SYSINFO_H
#define HEXCHAT_SYSINFO_H

#include <glib.h>

int sysinfo_get_cpu_arch (void);
int sysinfo_get_build_arch (void);
char *sysinfo_get_cpu (void);
char *sysinfo_get_os (void);
void sysinfo_get_hdd_info (guint64 *hdd_capacity_out, guint64 *hdd_free_space_out);
char *sysinfo_get_gpu (void);

#endif
