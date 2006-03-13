/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#define __APPLE_API_STRICT_CONFORMANCE

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <sys/timeb.h>
#include <process.h>
#else
#include <sys/types.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/utsname.h>
#endif
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include "xchat.h"
#include "xchatc.h"
#include <ctype.h>
#include "util.h"
#include "../../config.h"

#define WANTSOCKET
#include "inet.h"

#if defined (USING_FREEBSD) || defined (__APPLE__)
#include <sys/sysctl.h>
#endif
#ifdef SOCKS
#include <socks.h>
#endif

#ifndef HAVE_SNPRINTF
#define snprintf g_snprintf
#endif

#ifdef USE_DEBUG

#undef free
#undef malloc
#undef realloc
#undef strdup

int current_mem_usage;

struct mem_block
{
	char *file;
	void *buf;
	int size;
	int line;
	int total;
	struct mem_block *next;
};

struct mem_block *mroot = NULL;

void *
xchat_malloc (int size, char *file, int line)
{
	void *ret;
	struct mem_block *new;

	current_mem_usage += size;
	ret = malloc (size);
	if (!ret)
	{
		printf ("Out of memory! (%d)\n", current_mem_usage);
		exit (255);
	}

	new = malloc (sizeof (struct mem_block));
	new->buf = ret;
	new->size = size;
	new->next = mroot;
	new->line = line;
	new->file = strdup (file);
	mroot = new;

	printf ("%s:%d Malloc'ed %d bytes, now \033[35m%d\033[m\n", file, line,
				size, current_mem_usage);

	return ret;
}

void *
xchat_realloc (char *old, int len, char *file, int line)
{
	char *ret;

	ret = xchat_malloc (len, file, line);
	if (ret)
	{
		strcpy (ret, old);
		xchat_dfree (old, file, line);
	}
	return ret;
}

void *
xchat_strdup (char *str, char *file, int line)
{
	void *ret;
	struct mem_block *new;
	int size;

	size = strlen (str) + 1;
	current_mem_usage += size;
	ret = malloc (size);
	if (!ret)
	{
		printf ("Out of memory! (%d)\n", current_mem_usage);
		exit (255);
	}
	strcpy (ret, str);

	new = malloc (sizeof (struct mem_block));
	new->buf = ret;
	new->size = size;
	new->next = mroot;
	new->line = line;
	new->file = strdup (file);
	mroot = new;

	printf ("%s:%d strdup (\"%-.40s\") size: %d, total: \033[35m%d\033[m\n",
				file, line, str, size, current_mem_usage);

	return ret;
}

void
xchat_mem_list (void)
{
	struct mem_block *cur, *p;
	GSList *totals = 0;
	GSList *list;

	cur = mroot;
	while (cur)
	{
		list = totals;
		while (list)
		{
			p = list->data;
			if (p->line == cur->line &&
					strcmp (p->file, cur->file) == 0)
			{
				p->total += p->size;
				break;
			}
			list = list->next;
		}
		if (!list)
		{
			cur->total = cur->size;
			totals = g_slist_prepend (totals, cur);
		}
		cur = cur->next;
	}

	fprintf (stderr, "file              line   size    num  total\n");  
	list = totals;
	while (list)
	{
		cur = list->data;
		fprintf (stderr, "%-15.15s %6d %6d %6d %6d\n", cur->file, cur->line,
					cur->size, cur->total/cur->size, cur->total);
		list = list->next;
	}
}

void
xchat_dfree (void *buf, char *file, int line)
{
	struct mem_block *cur, *last;

	if (buf == NULL)
	{
		printf ("%s:%d \033[33mTried to free NULL\033[m\n", file, line);
		return;
	}

	last = NULL;
	cur = mroot;
	while (cur)
	{
		if (buf == cur->buf)
			break;
		last = cur;
		cur = cur->next;
	}
	if (cur == NULL)
	{
		printf ("%s:%d \033[31mTried to free unknown block %lx!\033[m\n",
				  file, line, (unsigned long) buf);
		/*      abort(); */
		free (buf);
		return;
	}
	current_mem_usage -= cur->size;
	printf ("%s:%d Free'ed %d bytes, usage now \033[35m%d\033[m\n",
				file, line, cur->size, current_mem_usage);
	if (last)
		last->next = cur->next;
	else
		mroot = cur->next;
	free (cur->file);
	free (cur);
}

#define malloc(n) xchat_malloc(n, __FILE__, __LINE__)
#define realloc(n, m) xchat_realloc(n, m, __FILE__, __LINE__)
#define free(n) xchat_dfree(n, __FILE__, __LINE__)
#define strdup(n) xchat_strdup(n, __FILE__, __LINE__)

#endif /* MEMORY_DEBUG */

char *
file_part (char *file)
{
	char *filepart = file;
	if (!file)
		return "";
	while (1)
	{
		switch (*file)
		{
		case 0:
			return (filepart);
		case '/':
#ifdef WIN32
		case '\\':
#endif
			filepart = file + 1;
			break;
		}
		file++;
	}
}

void
path_part (char *file, char *path, int pathlen)
{
	unsigned char t;
	char *filepart = file_part (file);
	t = *filepart;
	*filepart = 0;
	safe_strcpy (path, file, pathlen);
	*filepart = t;
}

char *				/* like strstr(), but nocase */
nocasestrstr (char *s, char *wanted)
{
	register const int len = strlen (wanted);

	if (len == 0)
		return (char *)s;
	while (rfc_tolower(*s) != rfc_tolower(*wanted) || strncasecmp (s, wanted, len))
		if (*s++ == '\0')
			return (char *)NULL;
	return (char *)s;
}

char *
errorstring (int err)
{
	switch (err)
	{
	case -1:
		return "";
	case 0:
		return _("Remote host closed socket");
#ifndef WIN32
	}
#else
	case WSAECONNREFUSED:
		return _("Connection refused");
	case WSAENETUNREACH:
	case WSAEHOSTUNREACH:
		return _("No route to host");
	case WSAETIMEDOUT:
		return _("Connection timed out");
	case WSAEADDRNOTAVAIL:
		return _("Cannot assign that address");
	case WSAECONNRESET:
		return _("Connection reset by peer");
	}

	/* can't use strerror() on Winsock errors! */
	if (err >= WSABASEERR)
	{
		static char tbuf[384];
		OSVERSIONINFO osvi;

		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		GetVersionEx (&osvi);

		/* FormatMessage works on WSA*** errors starting from Win2000 */
		if (osvi.dwMajorVersion >= 5)
		{
			if (FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM |
									  FORMAT_MESSAGE_IGNORE_INSERTS |
									  FORMAT_MESSAGE_MAX_WIDTH_MASK,
									  NULL, err,
									  MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
									  tbuf, sizeof (tbuf), NULL))
			{
				int len;
				char *utf;

				tbuf[sizeof (tbuf) - 1] = 0;
				len = strlen (tbuf);
				if (len >= 2)
					tbuf[len - 2] = 0;	/* remove the cr-lf */

				/* now convert to utf8 */
				utf = g_locale_to_utf8 (tbuf, -1, 0, 0, 0);
				if (utf)
				{
					safe_strcpy (tbuf, utf, sizeof (tbuf));
					g_free (utf);
					return tbuf;
				}
			}
		}	/* ! if (osvi.dwMajorVersion >= 5) */

		/* fallback to error number */
		sprintf (tbuf, "%s %d", _("Error"), err);
		return tbuf;
	} /* ! if (err >= WSABASEERR) */
#endif	/* ! WIN32 */

	return strerror (err);
}

int
waitline (int sok, char *buf, int bufsize, int use_recv)
{
	int i = 0;

	while (1)
	{
		if (use_recv)
		{
			if (recv (sok, &buf[i], 1, 0) < 1)
				return -1;
		} else
		{
			if (read (sok, &buf[i], 1) < 1)
				return -1;
		}
		if (buf[i] == '\n' || bufsize == i + 1)
		{
			buf[i] = 0;
			return i;
		}
		i++;
	}
}

/* checks for "~" in a file and expands */

char *
expand_homedir (char *file)
{
#ifndef WIN32
	char *ret, *user;
	struct passwd *pw;

	if (*file == '~')
	{
		if (file[1] != '\0' && file[1] != '/')
		{
			user = strdup(file);
			if (strchr(user,'/') != NULL)
				*(strchr(user,'/')) = '\0';
			if ((pw = getpwnam(user + 1)) == NULL)
			{
				free(user);
				return strdup(file);
			}
			free(user);
			user = strchr(file, '/') != NULL ? strchr(file,'/') : file;
			ret = malloc(strlen(user) + strlen(pw->pw_dir) + 1);
			strcpy(ret, pw->pw_dir);
			strcat(ret, user);
		}
		else
		{
			ret = malloc (strlen (file) + strlen (g_get_home_dir ()) + 1);
			sprintf (ret, "%s%s", g_get_home_dir (), file + 1);
		}
		return ret;
	}
#endif
	return strdup (file);
}

char *
strip_color (char *text, int len, int flags)
{
	char *new_str;

	if (len == -1)
		len = strlen (text);

	new_str = malloc (len + 2);
	strip_color2 (text, len, new_str, flags);
	return new_str;
}

/* CL: strip_color2 strips src and writes the output at dst; pass the same pointer
	in both arguments to strip in place. */
int
strip_color2 (char *src, int len, char *dst, int flags)
{
	int rcol = 0, bgcol = 0;
	char *start = dst;

	if (len == -1) len = strlen (src);
	while (len-- > 0)
	{
		if (rcol > 0 && (isdigit ((unsigned char)*src) ||
			(*src == ',' && isdigit ((unsigned char)src[1]) && !bgcol)))
		{
			if (src[1] != ',') rcol--;
			if (*src == ',')
			{
				rcol = 2;
				bgcol = 1;
			}
		} else
		{
			rcol = bgcol = 0;
			switch (*src)
			{
			case '\003':			  /*ATTR_COLOR: */
				if (!(flags & STRIP_COLOR)) goto pass_char;
				rcol = 2;
				break;
			case '\001':	/* CL: invisible text (for event formats only) */	/* this takes care of the topic */
				if (!(flags & STRIP_HIDDEN)) goto pass_char;
				break;
			case '\007':			  /*ATTR_BEEP: */
			case '\017':			  /*ATTR_RESET: */
			case '\026':			  /*ATTR_REVERSE: */
			case '\002':			  /*ATTR_BOLD: */
			case '\037':			  /*ATTR_UNDERLINE: */
			case '\035':			  /*ATTR_ITALICS: */
				if (!(flags & STRIP_ATTRIB)) goto pass_char;
				break;
			default:
			pass_char:
				*dst++ = *src;
			}
		}
		src++;
	}
	*dst = 0;

	return (int) (dst - start);
}

int
strip_hidden_attribute (char *src, char *dst)
{
	int len = 0;
	while (*src != '\000')
	{
		if (*src != '\001')
		{
			*dst++ = *src;
			len++;
		}
		src++;
	}
	return len;
}

#if defined (USING_LINUX) || defined (USING_FREEBSD) || defined (__APPLE__)

static void
get_cpu_info (double *mhz, int *cpus)
{

#ifdef USING_LINUX

	char buf[256];
	int fh;

	*mhz = 0;
	*cpus = 0;

	fh = open ("/proc/cpuinfo", O_RDONLY);	/* linux 2.2+ only */
	if (fh == -1)
	{
		*cpus = 1;
		return;
	}

	while (1)
	{
		if (waitline (fh, buf, sizeof buf, FALSE) < 0)
			break;
		if (!strncmp (buf, "cycle frequency [Hz]\t:", 22))	/* alpha */
		{
			*mhz = atoi (buf + 23) / 1048576;
		} else if (!strncmp (buf, "cpu MHz\t\t:", 10))	/* i386 */
		{
			*mhz = atof (buf + 11) + 0.5;
		} else if (!strncmp (buf, "clock\t\t:", 8))	/* PPC */
		{
			*mhz = atoi (buf + 9);
		} else if (!strncmp (buf, "processor\t", 10))
		{
			(*cpus)++;
		}
	}
	close (fh);
	if (!*cpus)
		*cpus = 1;

#endif
#ifdef USING_FREEBSD

	int mib[2], ncpu;
	u_long freq;
	size_t len;

	freq = 0;
	*mhz = 0;
	*cpus = 0;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;

	len = sizeof(ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);

	len = sizeof(freq);
	sysctlbyname("machdep.tsc_freq", &freq, &len, NULL, 0);

	*cpus = ncpu;
	*mhz = (freq / 1000000);

#endif
#ifdef __APPLE__

	int mib[2], ncpu;
	unsigned long long freq;
	size_t len;

	freq = 0;
	*mhz = 0;
	*cpus = 0;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;

	len = sizeof(ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);

	len = sizeof(freq);
        sysctlbyname("hw.cpufrequency", &freq, &len, NULL, 0);

	*cpus = ncpu;
	*mhz = (freq / 1000000);

#endif

}
#endif

#ifdef WIN32

static int
get_mhz (void)
{
	HKEY hKey;
	int result, data, dataSize;

	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\"
		"CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		dataSize = sizeof (data);
		result = RegQueryValueEx (hKey, "~MHz", 0, 0, (LPBYTE)&data, &dataSize);
		RegCloseKey (hKey);
		if (result == ERROR_SUCCESS)
			return data;
	}
	return 0;	/* fails on Win9x */
}

char *
get_cpu_str (void)
{
	static char verbuf[64];
	OSVERSIONINFO osvi;
	SYSTEM_INFO si;
	double mhz;

	osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	GetVersionEx (&osvi);
	GetSystemInfo (&si);

	mhz = get_mhz ();
	if (mhz)
	{
		double cpuspeed = ( mhz > 1000 ) ? mhz / 1000 : mhz;
		const char *cpuspeedstr = ( mhz > 1000 ) ? "GHz" : "MHz";
		sprintf (verbuf, "Windows %ld.%ld [i%d86/%.2f%s]",
					osvi.dwMajorVersion, osvi.dwMinorVersion, si.wProcessorLevel, 
					cpuspeed, cpuspeedstr);
	} else
		sprintf (verbuf, "Windows %ld.%ld [i%d86]",
			osvi.dwMajorVersion, osvi.dwMinorVersion, si.wProcessorLevel);

	return verbuf;
}

#else

char *
get_cpu_str (void)
{
#if defined (USING_LINUX) || defined (USING_FREEBSD) || defined (__APPLE__)
	double mhz;
#endif
	int cpus = 1;
	struct utsname un;
	static char *buf = NULL;

	if (buf)
		return buf;

	buf = malloc (128);

	uname (&un);

#if defined (USING_LINUX) || defined (USING_FREEBSD) || defined (__APPLE__)
	get_cpu_info (&mhz, &cpus);
	if (mhz)
	{
		double cpuspeed = ( mhz > 1000 ) ? mhz / 1000 : mhz;
		const char *cpuspeedstr = ( mhz > 1000 ) ? "GHz" : "MHz";
		snprintf (buf, 128,
					(cpus == 1) ? "%s %s [%s/%.2f%s]" : "%s %s [%s/%.2f%s/SMP]",
					un.sysname, un.release, un.machine,
					cpuspeed, cpuspeedstr);
	} else
#endif
		snprintf (buf, 128,
					(cpus == 1) ? "%s %s [%s]" : "%s %s [%s/SMP]",
					un.sysname, un.release, un.machine);

	return buf;
}

#endif

int
buf_get_line (char *ibuf, char **buf, int *position, int len)
{
	int pos = *position, spos = pos;

	if (pos == len)
		return 0;

	while (ibuf[pos++] != '\n')
	{
		if (pos == len)
			return 0;
	}
	pos--;
	ibuf[pos] = 0;
	*buf = &ibuf[spos];
	pos++;
	*position = pos;
	return 1;
}

int match(const char *mask, const char *string)
{
  register const char *m = mask, *s = string;
  register char ch;
  const char *bm, *bs;		/* Will be reg anyway on a decent CPU/compiler */

  /* Process the "head" of the mask, if any */
  while ((ch = *m++) && (ch != '*'))
    switch (ch)
    {
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	if (rfc_tolower(*s) != rfc_tolower(ch))
	  return 0;
      case '?':
	if (!*s++)
	  return 0;
    };
  if (!ch)
    return !(*s);

  /* We got a star: quickly find if/where we match the next char */
got_star:
  bm = m;			/* Next try rollback here */
  while ((ch = *m++))
    switch (ch)
    {
      case '?':
	if (!*s++)
	  return 0;
      case '*':
	bm = m;
	continue;		/* while */
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	goto break_while;	/* C is structured ? */
    };
break_while:
  if (!ch)
    return 1;			/* mask ends with '*', we got it */
  ch = rfc_tolower(ch);
  while (rfc_tolower(*s++) != ch)
    if (!*s)
      return 0;
  bs = s;			/* Next try start from here */

  /* Check the rest of the "chunk" */
  while ((ch = *m++))
  {
    switch (ch)
    {
      case '*':
	goto got_star;
      case '\\':
	if (*m == '?' || *m == '*')
	  ch = *m++;
      default:
	if (rfc_tolower(*s) != rfc_tolower(ch))
	{
	  m = bm;
	  s = bs;
	  goto got_star;
	};
      case '?':
	if (!*s++)
	  return 0;
    };
  };
  if (*s)
  {
    m = bm;
    s = bs;
    goto got_star;
  };
  return 1;
}

void
for_files (char *dirname, char *mask, void callback (char *file))
{
	DIR *dir;
	struct dirent *ent;
	char *buf;

	dir = opendir (dirname);
	if (dir)
	{
		while ((ent = readdir (dir)))
		{
			if (strcmp (ent->d_name, ".") && strcmp (ent->d_name, ".."))
			{
				if (match (mask, ent->d_name))
				{
					buf = malloc (strlen (dirname) + strlen (ent->d_name) + 2);
					sprintf (buf, "%s/%s", dirname, ent->d_name);
					callback (buf);
					free (buf);
				}
			}
		}
		closedir (dir);
	}
}

/*void
tolowerStr (char *str)
{
	while (*str)
	{
		*str = rfc_tolower (*str);
		str++;
	}
}*/

typedef struct
{
	char *code, *country;
} domain_t;

static int
country_compare (const void *a, const void *b)
{
	return strcasecmp (a, ((domain_t *)b)->code);
}

static const domain_t domain[] =
{
		{"AD", N_("Andorra") },
		{"AE", N_("United Arab Emirates") },
		{"AF", N_("Afghanistan") },
		{"AG", N_("Antigua and Barbuda") },
		{"AI", N_("Anguilla") },
		{"AL", N_("Albania") },
		{"AM", N_("Armenia") },
		{"AN", N_("Netherlands Antilles") },
		{"AO", N_("Angola") },
		{"AQ", N_("Antarctica") },
		{"AR", N_("Argentina") },
		{"ARPA", N_("Reverse DNS") },
		{"AS", N_("American Samoa") },
		{"AT", N_("Austria") },
		{"ATO", N_("Nato Fiel") },
		{"AU", N_("Australia") },
		{"AW", N_("Aruba") },
		{"AZ", N_("Azerbaijan") },
		{"BA", N_("Bosnia and Herzegovina") },
		{"BB", N_("Barbados") },
		{"BD", N_("Bangladesh") },
		{"BE", N_("Belgium") },
		{"BF", N_("Burkina Faso") },
		{"BG", N_("Bulgaria") },
		{"BH", N_("Bahrain") },
		{"BI", N_("Burundi") },
		{"BIZ", N_("Businesses"), },
		{"BJ", N_("Benin") },
		{"BM", N_("Bermuda") },
		{"BN", N_("Brunei Darussalam") },
		{"BO", N_("Bolivia") },
		{"BR", N_("Brazil") },
		{"BS", N_("Bahamas") },
		{"BT", N_("Bhutan") },
		{"BV", N_("Bouvet Island") },
		{"BW", N_("Botswana") },
		{"BY", N_("Belarus") },
		{"BZ", N_("Belize") },
		{"CA", N_("Canada") },
		{"CC", N_("Cocos Islands") },
		{"CD", N_("Democratic Republic of Congo") },
		{"CF", N_("Central African Republic") },
		{"CG", N_("Congo") },
		{"CH", N_("Switzerland") },
		{"CI", N_("Cote d'Ivoire") },
		{"CK", N_("Cook Islands") },
		{"CL", N_("Chile") },
		{"CM", N_("Cameroon") },
		{"CN", N_("China") },
		{"CO", N_("Colombia") },
		{"COM", N_("Internic Commercial") },
		{"CR", N_("Costa Rica") },
		{"CS", N_("Serbia and Montenegro") },
		{"CU", N_("Cuba") },
		{"CV", N_("Cape Verde") },
		{"CX", N_("Christmas Island") },
		{"CY", N_("Cyprus") },
		{"CZ", N_("Czech Republic") },
		{"DE", N_("Germany") },
		{"DJ", N_("Djibouti") },
		{"DK", N_("Denmark") },
		{"DM", N_("Dominica") },
		{"DO", N_("Dominican Republic") },
		{"DZ", N_("Algeria") },
		{"EC", N_("Ecuador") },
		{"EDU", N_("Educational Institution") },
		{"EE", N_("Estonia") },
		{"EG", N_("Egypt") },
		{"EH", N_("Western Sahara") },
		{"ER", N_("Eritrea") },
		{"ES", N_("Spain") },
		{"ET", N_("Ethiopia") },
		{"FI", N_("Finland") },
		{"FJ", N_("Fiji") },
		{"FK", N_("Falkland Islands") },
		{"FM", N_("Micronesia") },
		{"FO", N_("Faroe Islands") },
		{"FR", N_("France") },
		{"FX", N_("France, Metropolitan") },
		{"GA", N_("Gabon") },
		{"GB", N_("Great Britain") },
		{"GD", N_("Grenada") },
		{"GE", N_("Georgia") },
		{"GF", N_("French Guiana") },
		{"GG", N_("British Channel Isles") },
		{"GH", N_("Ghana") },
		{"GI", N_("Gibraltar") },
		{"GL", N_("Greenland") },
		{"GM", N_("Gambia") },
		{"GN", N_("Guinea") },
		{"GOV", N_("Government") },
		{"GP", N_("Guadeloupe") },
		{"GQ", N_("Equatorial Guinea") },
		{"GR", N_("Greece") },
		{"GS", N_("S. Georgia and S. Sandwich Isles") },
		{"GT", N_("Guatemala") },
		{"GU", N_("Guam") },
		{"GW", N_("Guinea-Bissau") },
		{"GY", N_("Guyana") },
		{"HK", N_("Hong Kong") },
		{"HM", N_("Heard and McDonald Islands") },
		{"HN", N_("Honduras") },
		{"HR", N_("Croatia") },
		{"HT", N_("Haiti") },
		{"HU", N_("Hungary") },
		{"ID", N_("Indonesia") },
		{"IE", N_("Ireland") },
		{"IL", N_("Israel") },
		{"IN", N_("India") },
		{"INFO", N_("Informational") },
		{"INT", N_("International") },
		{"IO", N_("British Indian Ocean Territory") },
		{"IQ", N_("Iraq") },
		{"IR", N_("Iran") },
		{"IS", N_("Iceland") },
		{"IT", N_("Italy") },
		{"JM", N_("Jamaica") },
		{"JO", N_("Jordan") },
		{"JP", N_("Japan") },
		{"KE", N_("Kenya") },
		{"KG", N_("Kyrgyzstan") },
		{"KH", N_("Cambodia") },
		{"KI", N_("Kiribati") },
		{"KM", N_("Comoros") },
		{"KN", N_("St. Kitts and Nevis") },
		{"KP", N_("North Korea") },
		{"KR", N_("South Korea") },
		{"KW", N_("Kuwait") },
		{"KY", N_("Cayman Islands") },
		{"KZ", N_("Kazakhstan") },
		{"LA", N_("Laos") },
		{"LB", N_("Lebanon") },
		{"LC", N_("Saint Lucia") },
		{"LI", N_("Liechtenstein") },
		{"LK", N_("Sri Lanka") },
		{"LR", N_("Liberia") },
		{"LS", N_("Lesotho") },
		{"LT", N_("Lithuania") },
		{"LU", N_("Luxembourg") },
		{"LV", N_("Latvia") },
		{"LY", N_("Libya") },
		{"MA", N_("Morocco") },
		{"MC", N_("Monaco") },
		{"MD", N_("Moldova") },
		{"MED", N_("United States Medical") },
		{"MG", N_("Madagascar") },
		{"MH", N_("Marshall Islands") },
		{"MIL", N_("Military") },
		{"MK", N_("Macedonia") },
		{"ML", N_("Mali") },
		{"MM", N_("Myanmar") },
		{"MN", N_("Mongolia") },
		{"MO", N_("Macau") },
		{"MP", N_("Northern Mariana Islands") },
		{"MQ", N_("Martinique") },
		{"MR", N_("Mauritania") },
		{"MS", N_("Montserrat") },
		{"MT", N_("Malta") },
		{"MU", N_("Mauritius") },
		{"MV", N_("Maldives") },
		{"MW", N_("Malawi") },
		{"MX", N_("Mexico") },
		{"MY", N_("Malaysia") },
		{"MZ", N_("Mozambique") },
		{"NA", N_("Namibia") },
		{"NC", N_("New Caledonia") },
		{"NE", N_("Niger") },
		{"NET", N_("Internic Network") },
		{"NF", N_("Norfolk Island") },
		{"NG", N_("Nigeria") },
		{"NI", N_("Nicaragua") },
		{"NL", N_("Netherlands") },
		{"NO", N_("Norway") },
		{"NP", N_("Nepal") },
		{"NR", N_("Nauru") },
		{"NT", N_("Neutral Zone") },
		{"NU", N_("Niue") },
		{"NZ", N_("New Zealand") },
		{"OM", N_("Oman") },
		{"ORG", N_("Internic Non-Profit Organization") },
		{"PA", N_("Panama") },
		{"PE", N_("Peru") },
		{"PF", N_("French Polynesia") },
		{"PG", N_("Papua New Guinea") },
		{"PH", N_("Philippines") },
		{"PK", N_("Pakistan") },
		{"PL", N_("Poland") },
		{"PM", N_("St. Pierre and Miquelon") },
		{"PN", N_("Pitcairn") },
		{"PR", N_("Puerto Rico") },
		{"PS", N_("Palestinian Territory") },
		{"PT", N_("Portugal") },
		{"PW", N_("Palau") },
		{"PY", N_("Paraguay") },
		{"QA", N_("Qatar") },
		{"RE", N_("Reunion") },
		{"RO", N_("Romania") },
		{"RPA", N_("Old School ARPAnet") },
		{"RU", N_("Russian Federation") },
		{"RW", N_("Rwanda") },
		{"SA", N_("Saudi Arabia") },
		{"SB", N_("Solomon Islands") },
		{"SC", N_("Seychelles") },
		{"SD", N_("Sudan") },
		{"SE", N_("Sweden") },
		{"SG", N_("Singapore") },
		{"SH", N_("St. Helena") },
		{"SI", N_("Slovenia") },
		{"SJ", N_("Svalbard and Jan Mayen Islands") },
		{"SK", N_("Slovak Republic") },
		{"SL", N_("Sierra Leone") },
		{"SM", N_("San Marino") },
		{"SN", N_("Senegal") },
		{"SO", N_("Somalia") },
		{"SR", N_("Suriname") },
		{"ST", N_("Sao Tome and Principe") },
		{"SU", N_("Former USSR") },
		{"SV", N_("El Salvador") },
		{"SY", N_("Syria") },
		{"SZ", N_("Swaziland") },
		{"TC", N_("Turks and Caicos Islands") },
		{"TD", N_("Chad") },
		{"TF", N_("French Southern Territories") },
		{"TG", N_("Togo") },
		{"TH", N_("Thailand") },
		{"TJ", N_("Tajikistan") },
		{"TK", N_("Tokelau") },
		{"TM", N_("Turkmenistan") },
		{"TN", N_("Tunisia") },
		{"TO", N_("Tonga") },
		{"TP", N_("East Timor") },
		{"TR", N_("Turkey") },
		{"TT", N_("Trinidad and Tobago") },
		{"TV", N_("Tuvalu") },
		{"TW", N_("Taiwan") },
		{"TZ", N_("Tanzania") },
		{"UA", N_("Ukraine") },
		{"UG", N_("Uganda") },
		{"UK", N_("United Kingdom") },
		{"UM", N_("US Minor Outlying Islands") },
		{"US", N_("United States of America") },
		{"UY", N_("Uruguay") },
		{"UZ", N_("Uzbekistan") },
		{"VA", N_("Vatican City State") },
		{"VC", N_("St. Vincent and the Grenadines") },
		{"VE", N_("Venezuela") },
		{"VG", N_("British Virgin Islands") },
		{"VI", N_("US Virgin Islands") },
		{"VN", N_("Vietnam") },
		{"VU", N_("Vanuatu") },
		{"WF", N_("Wallis and Futuna Islands") },
		{"WS", N_("Samoa") },
		{"YE", N_("Yemen") },
		{"YT", N_("Mayotte") },
		{"YU", N_("Yugoslavia") },
		{"ZA", N_("South Africa") },
		{"ZM", N_("Zambia") },
		{"ZW", N_("Zimbabwe") },
};

char *
country (char *hostname)
{
	char *p;
	domain_t *dom;

	if (!hostname || !*hostname || isdigit ((unsigned char) hostname[strlen (hostname) - 1]))
		return _("Unknown");
	if ((p = strrchr (hostname, '.')))
		p++;
	else
		p = hostname;

	dom = bsearch (p, domain, sizeof (domain) / sizeof (domain_t),
						sizeof (domain_t), country_compare);

	if (!dom)
		return _("Unknown");

	return _(dom->country);
}

void
country_search (char *pattern, void *ud, void (*print)(void *, char *, ...))
{
	const domain_t *dom;
	int i;

	for (i = 0; i < sizeof (domain) / sizeof (domain_t); i++)
	{
		dom = &domain[i];
		if (match (pattern, dom->country) || match (pattern, _(dom->country)))
		{
			print (ud, "%s = %s\n", dom->code, _(dom->country));
		}
	}
}

/* I think gnome1.0.x isn't necessarily linked against popt, ah well! */
/* !!! For now use this inlined function, or it would break fe-text building */
/* .... will find a better solution later. */
/*#ifndef USE_GNOME*/

/* this is taken from gnome-libs 1.2.4 */
#define POPT_ARGV_ARRAY_GROW_DELTA 5

int my_poptParseArgvString(const char * s, int * argcPtr, char *** argvPtr) {
    char * buf, * bufStart, * dst;
    const char * src;
    char quote = '\0';
    int argvAlloced = POPT_ARGV_ARRAY_GROW_DELTA;
    char ** argv = malloc(sizeof(*argv) * argvAlloced);
    const char ** argv2;
    int argc = 0;
    int i, buflen;

    buflen = strlen(s) + 1;
/*    bufStart = buf = alloca(buflen);*/
	 bufStart = buf = malloc (buflen);
    memset(buf, '\0', buflen);

    src = s;
    argv[argc] = buf;

    while (*src) {
	if (quote == *src) {
	    quote = '\0';
	} else if (quote) {
	    if (*src == '\\') {
		src++;
		if (!*src) {
		    free(argv);
			 free(bufStart);
		    return 1;
		}
		if (*src != quote) *buf++ = '\\';
	    }
	    *buf++ = *src;
	} else if (isspace((unsigned char) *src)) {
	    if (*argv[argc]) {
		buf++, argc++;
		if (argc == argvAlloced) {
		    argvAlloced += POPT_ARGV_ARRAY_GROW_DELTA;
		    argv = realloc(argv, sizeof(*argv) * argvAlloced);
		}
		argv[argc] = buf;
	    }
	} else switch (*src) {
	  case '"':
	  case '\'':
	    quote = *src;
	    break;
	  case '\\':
	    src++;
	    if (!*src) {
		free(argv);
		free(bufStart);
		return 1;
	    }
	    /* fallthrough */
	  default:
	    *buf++ = *src;
	}

	src++;
    }

    if (strlen(argv[argc])) {
	argc++, buf++;
    }

    dst = malloc((argc + 1) * sizeof(*argv) + (buf - bufStart));
    argv2 = (void *) dst;
    dst += (argc + 1) * sizeof(*argv);
    memcpy((void *)argv2, argv, argc * sizeof(*argv));
    argv2[argc] = NULL;
    memcpy(dst, bufStart, buf - bufStart);

    for (i = 0; i < argc; i++) {
	argv2[i] = dst + (argv[i] - bufStart);
    }

    free(argv);

    *argvPtr = (char **)argv2;	/* XXX don't change the API */
    *argcPtr = argc;

	 free (bufStart);

    return 0;
}

int
util_exec (char *cmd)
{
	int pid;
	char **argv;
	int argc;
	int fd;

	if (my_poptParseArgvString (cmd, &argc, &argv) != 0)
		return -1;

#ifndef WIN32
	pid = fork ();
	if (pid == -1)
		return -1;
	if (pid == 0)
	{
		/* Now close all open file descriptors except stdin, stdout and stderr */
		for (fd = 3; fd < 1024; fd++) close(fd);
		execvp (argv[0], argv);
		_exit (0);
	} else
	{
		free (argv);
		return pid;
	}
#else
	spawnvp (_P_DETACH, argv[0], argv);
	free (argv);
	return 0;
#endif
}

unsigned long
make_ping_time (void)
{
#ifndef WIN32
	struct timeval timev;
	gettimeofday (&timev, 0);
#else
	GTimeVal timev;
	g_get_current_time (&timev);
#endif
	return (timev.tv_sec - 50000) * 1000000 + timev.tv_usec;
}


/************************************************************************
 *    This technique was borrowed in part from the source code to 
 *    ircd-hybrid-5.3 to implement case-insensitive string matches which
 *    are fully compliant with Section 2.2 of RFC 1459, the copyright
 *    of that code being (C) 1990 Jarkko Oikarinen and under the GPL.
 *    
 *    A special thanks goes to Mr. Okarinen for being the one person who
 *    seems to have ever noticed this section in the original RFC and
 *    written code for it.  Shame on all the rest of you (myself included).
 *    
 *        --+ Dagmar d'Surreal
 */

int
rfc_casecmp (const char *s1, const char *s2)
{
	register unsigned char *str1 = (unsigned char *) s1;
	register unsigned char *str2 = (unsigned char *) s2;
	register int res;

	while ((res = rfc_tolower (*str1) - rfc_tolower (*str2)) == 0)
	{
		if (*str1 == '\0')
			return 0;
		str1++;
		str2++;
	}
	return (res);
}

int
rfc_ncasecmp (char *str1, char *str2, int n)
{
	register unsigned char *s1 = (unsigned char *) str1;
	register unsigned char *s2 = (unsigned char *) str2;
	register int res;

	while ((res = rfc_tolower (*s1) - rfc_tolower (*s2)) == 0)
	{
		s1++;
		s2++;
		n--;
		if (n == 0 || (*s1 == '\0' && *s2 == '\0'))
			return 0;
	}
	return (res);
}

const unsigned char rfc_tolowertab[] =
	{ 0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	'_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

/*static unsigned char touppertab[] =
	{ 0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
	0x5f,
	'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};*/

/*static int
rename_utf8 (char *oldname, char *newname)
{
	int sav, res;
	char *fso, *fsn;

	fso = g_filename_from_utf8 (oldname, -1, 0, 0, 0);
	if (!fso)
		return FALSE;
	fsn = g_filename_from_utf8 (newname, -1, 0, 0, 0);
	if (!fsn)
	{
		g_free (fso);
		return FALSE;
	}

	res = rename (fso, fsn);
	sav = errno;
	g_free (fso);
	g_free (fsn);
	errno = sav;
	return res;
}

static int
unlink_utf8 (char *fname)
{
	int res;
	char *fs;

	fs = g_filename_from_utf8 (fname, -1, 0, 0, 0);
	if (!fs)
		return FALSE;

	res = unlink (fs);
	g_free (fs);
	return res;
}*/

static gboolean
file_exists_utf8 (char *fname)
{
	int res;
	char *fs;

	fs = g_filename_from_utf8 (fname, -1, 0, 0, 0);
	if (!fs)
		return FALSE;

	res = access (fs, F_OK);
	g_free (fs);
	if (res == 0)
		return TRUE;
	return FALSE;
}

static gboolean
copy_file (char *dl_src, char *dl_dest, int permissions)	/* FS encoding */
{
	int tmp_src, tmp_dest;
	gboolean ok = FALSE;
	char dl_tmp[4096];
	int return_tmp, return_tmp2;

	if ((tmp_src = open (dl_src, O_RDONLY | OFLAGS)) == -1)
	{
		fprintf (stderr, "Unable to open() file '%s' (%s) !", dl_src,
				  strerror (errno));
		return FALSE;
	}

	if ((tmp_dest =
		 open (dl_dest, O_WRONLY | O_CREAT | O_TRUNC | OFLAGS, permissions)) < 0)
	{
		close (tmp_src);
		fprintf (stderr, "Unable to create file '%s' (%s) !", dl_src,
				  strerror (errno));
		return FALSE;
	}

	for (;;)
	{
		return_tmp = read (tmp_src, dl_tmp, sizeof (dl_tmp));

		if (!return_tmp)
		{
			ok = TRUE;
			break;
		}

		if (return_tmp < 0)
		{
			fprintf (stderr, "download_move_to_completed_dir(): "
				"error reading while moving file to save directory (%s)",
				 strerror (errno));
			break;
		}

		return_tmp2 = write (tmp_dest, dl_tmp, return_tmp);

		if (return_tmp2 < 0)
		{
			fprintf (stderr, "download_move_to_completed_dir(): "
				"error writing while moving file to save directory (%s)",
				 strerror (errno));
			break;
		}

		if (return_tmp < sizeof (dl_tmp))
		{
			ok = TRUE;
			break;
		}
	}

	close (tmp_dest);
	close (tmp_src);
	return ok;
}

/* Takes care of moving a file from a temporary download location to a completed location. Now in UTF-8. */
void
move_file_utf8 (char *src_dir, char *dst_dir, char *fname, int dccpermissions)
{
	char src[4096];
	char dst[4096];
	int res, i;
	char *src_fs;	/* FileSystem encoding */
	char *dst_fs;

	/* if dcc_dir and dcc_completed_dir are the same then we are done */
	if (0 == strcmp (src_dir, dst_dir) ||
		 0 == dst_dir[0])
		return;			/* Already in "completed dir" */

	snprintf (src, sizeof (src), "%s/%s", src_dir, fname);
	snprintf (dst, sizeof (dst), "%s/%s", dst_dir, fname);

	/* already exists in completed dir? Append a number */
	if (file_exists_utf8 (dst))
	{
		for (i = 0; ; i++)
		{
			snprintf (dst, sizeof (dst), "%s/%s.%d", dst_dir, fname, i);
			if (!file_exists_utf8 (dst))
				break;
		}
	}

	/* convert UTF-8 to filesystem encoding */
	src_fs = g_filename_from_utf8 (src, -1, 0, 0, 0);
	if (!src_fs)
		return;
	dst_fs = g_filename_from_utf8 (dst, -1, 0, 0, 0);
	if (!dst_fs)
	{
		g_free (src_fs);
		return;
	}

	/* first try a simple rename move */
	res = rename (src_fs, dst_fs);

	if (res == -1 && (errno == EXDEV || errno == EPERM))
	{
		/* link failed because either the two paths aren't on the */
		/* same filesystem or the filesystem doesn't support hard */
		/* links, so we have to do a copy. */
		if (copy_file (src_fs, dst_fs, dccpermissions))
			unlink (src_fs);
	}

	g_free (dst_fs);
	g_free (src_fs);
}

int
mkdir_utf8 (char *dir)
{
	int ret;

	dir = g_filename_from_utf8 (dir, -1, 0, 0, 0);
	if (!dir)
		return -1;

#ifdef WIN32
	ret = mkdir (dir);
#else
	ret = mkdir (dir, S_IRUSR | S_IWUSR | S_IXUSR);
#endif
	g_free (dir);

	return ret;
}

/* separates a string according to a 'sep' char, then calls the callback
   function for each token found */

int
token_foreach (char *str, char sep,
					int (*callback) (char *str, void *ud), void *ud)
{
	char t, *start = str;

	while (1)
	{
		if (*str == sep || *str == 0)
		{
			t = *str;
			*str = 0;
			if (callback (start, ud) < 1)
			{
				*str = t;
				return FALSE;
			}
			*str = t;

			if (*str == 0)
				break;
			str++;
			start = str;

		} else
		{
			/* chars $00-$7f can never be embedded in utf-8 */
			str++;
		}
	}

	return TRUE;
}
