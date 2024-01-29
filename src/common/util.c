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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WANTSOCKET
#include "inet.h"				/* make it first to avoid macro redefinitions */

#define __APPLE_API_STRICT_CONFORMANCE

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <sys/timeb.h>
#include <io.h>
#include "./sysinfo/sysinfo.h"
#else
#include <unistd.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/utsname.h>
#endif

#include "config.h"
#include <fcntl.h>
#include <errno.h>
#include "hexchat.h"
#include "hexchatc.h"
#include <ctype.h>
#include "util.h"

#if defined (__FreeBSD__) || defined (__APPLE__)
#include <sys/sysctl.h>
#endif

/* SASL mechanisms */
#ifdef USE_OPENSSL
#include <openssl/bn.h>
#include <openssl/rand.h>
#ifndef WIN32
#include <netinet/in.h>
#endif
#endif

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
nocasestrstr (const char *s, const char *wanted)
{
	register const int len = strlen (wanted);

	if (len == 0)
		return (char *)s;
	while (rfc_tolower(*s) != rfc_tolower(*wanted) || g_ascii_strncasecmp (s, wanted, len))
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

#ifdef WIN32
/* waitline2 using win32 file descriptor and glib instead of _read. win32 can't _read() sok! */
int
waitline2 (GIOChannel *source, char *buf, int bufsize)
{
	int i = 0;
	gsize len;
	GError *error = NULL;

	while (1)
	{
		g_io_channel_set_buffered (source, FALSE);
		g_io_channel_set_encoding (source, NULL, &error);

		if (g_io_channel_read_chars (source, &buf[i], 1, &len, &error) != G_IO_STATUS_NORMAL)
		{
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
#endif

/* checks for "~" in a file and expands */

char *
expand_homedir (char *file)
{
#ifndef WIN32
	char *user;
	struct passwd *pw;

	if (file[0] == '~')
	{
		char *slash_pos;

		if (file[1] == '\0' || file[1] == '/')
			return g_strconcat (g_get_home_dir (), &file[1], NULL);

		user = g_strdup(file);

		slash_pos = strchr(user, '/');
		if (slash_pos != NULL)
			*slash_pos = '\0';

		pw = getpwnam(user + 1);
		g_free(user);

		if (pw == NULL)
			return g_strdup(file);

		slash_pos = strchr(file, '/');
		if (slash_pos == NULL)
			return g_strdup (pw->pw_dir);
		else
			return g_strconcat (pw->pw_dir, slash_pos, NULL);
	}
#endif
	return g_strdup (file);
}

gchar *
strip_color (const char *text, int len, int flags)
{
	char *new_str;

	if (len == -1)
		len = strlen (text);

	new_str = g_malloc (len + 2);
	strip_color2 (text, len, new_str, flags);

	if (flags & STRIP_ESCMARKUP)
	{
		char *esc = g_markup_escape_text (new_str, -1);
		g_free (new_str);
		return esc;
	}

	return new_str;
}

/* CL: strip_color2 strips src and writes the output at dst; pass the same pointer
	in both arguments to strip in place. */
int
strip_color2 (const char *src, int len, char *dst, int flags)
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
			case HIDDEN_CHAR:	/* CL: invisible text (for event formats only) */	/* this takes care of the topic */
				if (!(flags & STRIP_HIDDEN)) goto pass_char;
				break;
			case '\007':			  /*ATTR_BEEP: */
			case '\017':			  /*ATTR_RESET: */
			case '\026':			  /*ATTR_REVERSE: */
			case '\002':			  /*ATTR_BOLD: */
			case '\037':			  /*ATTR_UNDERLINE: */
			case '\036':			  /*ATTR_STRIKETHROUGH: */
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
		if (*src != HIDDEN_CHAR)
		{
			*dst++ = *src;
			len++;
		}
		src++;
	}
	return len;
}

#if defined (__linux__) || defined (__FreeBSD__) || defined (__APPLE__) || defined (__CYGWIN__)

static void
get_cpu_info (double *mhz, int *cpus)
{

#if defined(__linux__) || defined (__CYGWIN__)

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
			*mhz = atoi (buf + 23) / 1000000;
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
#ifdef __FreeBSD__

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

int
get_cpu_arch (void)
{
	return sysinfo_get_build_arch ();
}

char *
get_sys_str (int with_cpu)
{
	static char *without_cpu_buffer = NULL;
	static char *with_cpu_buffer = NULL;

	if (with_cpu == 0)
	{
		if (without_cpu_buffer == NULL)
		{
			without_cpu_buffer = sysinfo_get_os ();
		}

		return without_cpu_buffer;
	}

	if (with_cpu_buffer == NULL)
	{
		char *os = sysinfo_get_os ();
		char *cpu = sysinfo_get_cpu ();
		with_cpu_buffer = g_strconcat (os, " [", cpu, "]", NULL);
		g_free (cpu);
		g_free (os);
	}

	return with_cpu_buffer;
}

#else

char *
get_sys_str (int with_cpu)
{
#if defined (__linux__) || defined (__FreeBSD__) || defined (__APPLE__) || defined (__CYGWIN__)
	double mhz;
#endif
	int cpus = 1;
	struct utsname un;
	static char *buf = NULL;

	if (buf)
		return buf;

	uname (&un);

#if defined (__linux__) || defined (__FreeBSD__) || defined (__APPLE__) || defined (__CYGWIN__)
	get_cpu_info (&mhz, &cpus);
	if (mhz && with_cpu)
	{
		double cpuspeed = ( mhz > 1000 ) ? mhz / 1000 : mhz;
		const char *cpuspeedstr = ( mhz > 1000 ) ? "GHz" : "MHz";
		buf = g_strdup_printf (
			(cpus == 1) ? "%s %s [%s/%.2f%s]" : "%s %s [%s/%.2f%s/SMP]",
			un.sysname, un.release, un.machine,
			cpuspeed, cpuspeedstr);
	}
	else
		buf = g_strdup_printf ("%s %s", un.sysname, un.release);
#else
	buf = g_strdup_printf ("%s %s", un.sysname, un.release);
#endif

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
	  if (!*s)
	    return 0;
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
for_files (const char *dirname, const char *mask, void callback (char *file))
{
	GDir *dir;
	const gchar *entry_name;
	char *buf;

	dir = g_dir_open (dirname, 0, NULL);
	if (dir)
	{
		while ((entry_name = g_dir_read_name (dir)))
		{
			if (strcmp (entry_name, ".") && strcmp (entry_name, ".."))
			{
				if (match (mask, entry_name))
				{
					buf = g_build_filename (dirname, entry_name, NULL);
					callback (buf);
					g_free (buf);
				}
			}
		}
		g_dir_close (dir);
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
	return g_ascii_strcasecmp (a, ((domain_t *)b)->code);
}

static const domain_t domain[] =
{
		{"AC", N_("Ascension Island") },
		{"AD", N_("Andorra") },
		{"AE", N_("United Arab Emirates") },
		{"AERO", N_("Aviation-Related Fields") },
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
		{"ASIA", N_("Asia-Pacific Region") },
		{"AT", N_("Austria") },
		{"ATO", N_("Nato Fiel") },
		{"AU", N_("Australia") },
		{"AW", N_("Aruba") },
		{"AX", N_("Aland Islands") },
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
		{"CAT", N_("Catalan") },
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
		{"COOP", N_("Cooperatives") },
		{"CR", N_("Costa Rica") },
		{"CS", N_("Serbia and Montenegro") },
		{"CU", N_("Cuba") },
		{"CV", N_("Cape Verde") },
		{"CX", N_("Christmas Island") },
		{"CY", N_("Cyprus") },
		{"CZ", N_("Czech Republic") },
		{"DD", N_("East Germany") },
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
		{"EU", N_("European Union") },
		{"FI", N_("Finland") },
		{"FJ", N_("Fiji") },
		{"FK", N_("Falkland Islands") },
		{"FM", N_("Micronesia") },
		{"FO", N_("Faroe Islands") },
		{"FR", N_("France") },
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
		{"IM", N_("Isle of Man") },
		{"IN", N_("India") },
		{"INFO", N_("Informational") },
		{"INT", N_("International") },
		{"IO", N_("British Indian Ocean Territory") },
		{"IQ", N_("Iraq") },
		{"IR", N_("Iran") },
		{"IS", N_("Iceland") },
		{"IT", N_("Italy") },
		{"JE", N_("Jersey") },
		{"JM", N_("Jamaica") },
		{"JO", N_("Jordan") },
		{"JOBS", N_("Company Jobs") },
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
		{"ME", N_("Montenegro") },
		{"MED", N_("United States Medical") },
		{"MG", N_("Madagascar") },
		{"MH", N_("Marshall Islands") },
		{"MIL", N_("Military") },
		{"MK", N_("Macedonia") },
		{"ML", N_("Mali") },
		{"MM", N_("Myanmar") },
		{"MN", N_("Mongolia") },
		{"MO", N_("Macau") },
		{"MOBI", N_("Mobile Devices") },
		{"MP", N_("Northern Mariana Islands") },
		{"MQ", N_("Martinique") },
		{"MR", N_("Mauritania") },
		{"MS", N_("Montserrat") },
		{"MT", N_("Malta") },
		{"MU", N_("Mauritius") },
		{"MUSEUM", N_("Museums") },
		{"MV", N_("Maldives") },
		{"MW", N_("Malawi") },
		{"MX", N_("Mexico") },
		{"MY", N_("Malaysia") },
		{"MZ", N_("Mozambique") },
		{"NA", N_("Namibia") },
		{"NAME", N_("Individual's Names") },
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
		{"PRO", N_("Professions") },
		{"PS", N_("Palestinian Territory") },
		{"PT", N_("Portugal") },
		{"PW", N_("Palau") },
		{"PY", N_("Paraguay") },
		{"QA", N_("Qatar") },
		{"RE", N_("Reunion") },
		{"RO", N_("Romania") },
		{"RPA", N_("Old School ARPAnet") },
		{"RS", N_("Serbia") },
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
		{"SS", N_("South Sudan") },
		{"ST", N_("Sao Tome and Principe") },
		{"SU", N_("Former USSR") },
		{"SV", N_("El Salvador") },
		{"SY", N_("Syria") },
		{"SZ", N_("Swaziland") },
		{"TC", N_("Turks and Caicos Islands") },
		{"TD", N_("Chad") },
		{"TEL", N_("Internet Communication Services") },
		{"TF", N_("French Southern Territories") },
		{"TG", N_("Togo") },
		{"TH", N_("Thailand") },
		{"TJ", N_("Tajikistan") },
		{"TK", N_("Tokelau") },
		{"TL", N_("East Timor") },
		{"TM", N_("Turkmenistan") },
		{"TN", N_("Tunisia") },
		{"TO", N_("Tonga") },
		{"TP", N_("East Timor") },
		{"TR", N_("Turkey") },
		{"TRAVEL", N_("Travel and Tourism") },
		{"TT", N_("Trinidad and Tobago") },
		{"TV", N_("Tuvalu") },
		{"TW", N_("Taiwan") },
		{"TZ", N_("Tanzania") },
		{"UA", N_("Ukraine") },
		{"UG", N_("Uganda") },
		{"UK", N_("United Kingdom") },
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
		{"XXX", N_("Adult Entertainment") },
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
		return NULL;
	if ((p = strrchr (hostname, '.')))
		p++;
	else
		p = hostname;

	dom = bsearch (p, domain, sizeof (domain) / sizeof (domain_t),
						sizeof (domain_t), country_compare);

	if (!dom)
		return NULL;

	return _(dom->country);
}

void
country_search (char *pattern, void *ud, void (*print)(void *, char *, ...))
{
	const domain_t *dom;
	size_t i;

	for (i = 0; i < sizeof (domain) / sizeof (domain_t); i++)
	{
		dom = &domain[i];
		if (match (pattern, dom->country) || match (pattern, _(dom->country)))
		{
			print (ud, "%s = %s\n", dom->code, _(dom->country));
		}
	}
}

void
util_exec (const char *cmd)
{
	g_spawn_command_line_async (cmd, NULL);
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
	return (timev.tv_sec - 50000) * 1000 + timev.tv_usec/1000;
}

int
rfc_casecmp (const char *s1, const char *s2)
{
	int c1, c2;

	while (*s1 && *s2)
	{
		c1 = (int)rfc_tolower (*s1);
		c2 = (int)rfc_tolower (*s2);
		if (c1 != c2)
			return (c1 - c2);
		s1++;
		s2++;
	}
	return (((int)*s1) - ((int)*s2));
}

int
rfc_ncasecmp (char *s1, char *s2, int n)
{
	int c1, c2;

	while (*s1 && *s2 && n > 0)
	{
		c1 = (int)rfc_tolower (*s1);
		c2 = (int)rfc_tolower (*s2);
		if (c1 != c2)
			return (c1 - c2);
		n--;
		s1++;
		s2++;
	}
	c1 = (int)rfc_tolower (*s1);
	c2 = (int)rfc_tolower (*s2);
	return (n == 0) ? 0 : (c1 - c2);
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

static gboolean
file_exists (char *fname)
{
	return (g_access (fname, F_OK) == 0) ? TRUE : FALSE;
}

static gboolean
copy_file (char *dl_src, char *dl_dest, int permissions)
{
	int tmp_src, tmp_dest;
	gboolean ok = FALSE;
	char dl_tmp[4096];
	int return_tmp, return_tmp2;

	if ((tmp_src = g_open (dl_src, O_RDONLY | OFLAGS, 0600)) == -1)
	{
		g_fprintf (stderr, "Unable to open() file '%s' (%s) !", dl_src,
				  strerror (errno));
		return FALSE;
	}

	if ((tmp_dest =
		 g_open (dl_dest, O_WRONLY | O_CREAT | O_TRUNC | OFLAGS, permissions)) < 0)
	{
		close (tmp_src);
		g_fprintf (stderr, "Unable to create file '%s' (%s) !", dl_src,
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

/* Takes care of moving a file from a temporary download location to a completed location. */
void
move_file (char *src_dir, char *dst_dir, char *fname, int dccpermissions)
{
	char *src;
	char *dst;
	int res, i;

	/* if dcc_dir and dcc_completed_dir are the same then we are done */
	if (0 == strcmp (src_dir, dst_dir) ||
		 0 == dst_dir[0])
		return;			/* Already in "completed dir" */

	src = g_build_filename (src_dir, fname, NULL);
	dst = g_build_filename (dst_dir, fname, NULL);

	/* already exists in completed dir? Append a number */
	if (file_exists (dst))
	{
		for (i = 0; ; i++)
		{
			g_free (dst);
			dst = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s.%d", dst_dir, fname, i);
			if (!file_exists (dst))
				break;
		}
	}

	/* first try a simple rename move */
	res = g_rename (src, dst);

	if (res == -1 && (errno == EXDEV || errno == EPERM))
	{
		/* link failed because either the two paths aren't on the */
		/* same filesystem or the filesystem doesn't support hard */
		/* links, so we have to do a copy. */
		if (copy_file (src, dst, dccpermissions))
			g_unlink (src);
	}

	g_free (dst);
	g_free (src);
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

/* 31 bit string hash functions */

guint32
str_hash (const char *key)
{
	const char *p = key;
	guint32 h = *p;

	if (h)
		for (p += 1; *p != '\0'; p++)
			h = (h << 5) - h + *p;

	return h;
}

guint32
str_ihash (const unsigned char *key)
{
	const char *p = key;
	guint32 h = rfc_tolowertab [(guint)*p];

	if (h)
		for (p += 1; *p != '\0'; p++)
			h = (h << 5) - h + rfc_tolowertab [(guint)*p];

	return h;
}

/* features: 1. "src" must be valid, NULL terminated UTF-8 */
/*           2. "dest" will be left with valid UTF-8 - no partial chars! */

void
safe_strcpy (char *dest, const char *src, int bytes_left)
{
	int mbl;

	while (1)
	{
		mbl = g_utf8_skip[*((unsigned char *)src)];

		if (bytes_left < (mbl + 1)) /* can't fit with NULL? */
		{
			*dest = 0;
			break;
		}

		if (mbl == 1)	/* one byte char */
		{
			*dest = *src;
			if (*src == 0)
				break;	/* it all fit */
			dest++;
			src++;
			bytes_left--;
		}
		else				/* multibyte char */
		{
			memcpy (dest, src, mbl);
			dest += mbl;
			src += mbl;
			bytes_left -= mbl;
		}
	}
}

void
canonalize_key (char *key)
{
	char *pos, token;

	for (pos = key; (token = *pos) != 0; pos++)
	{
		if (token != '_' && (token < '0' || token > '9') && (token < 'A' || token > 'Z') && (token < 'a' || token > 'z'))
		{
			*pos = '_';
		}
		else
		{
			*pos = tolower(token);
		}
	}
}

int
portable_mode (void)
{
#ifdef WIN32
	static int is_portable = -1;

	if (G_UNLIKELY(is_portable == -1))
	{
		char *path = g_win32_get_package_installation_directory_of_module (NULL);
		char *filename;

		if (path == NULL)
			path = g_strdup (".");

		filename = g_build_filename (path, "portable-mode", NULL);
		is_portable = g_file_test (filename, G_FILE_TEST_EXISTS);

		g_free (path);
		g_free (filename);
	}

	return is_portable;
#else
	return 0;
#endif
}

char *
encode_sasl_pass_plain (char *user, char *pass)
{
	int authlen;
	char *buffer;
	char *encoded;

	/* we can't call strlen() directly on buffer thanks to the atrocious \0 characters it requires */
	authlen = strlen (user) * 2 + 2 + strlen (pass);
	buffer = g_strdup_printf ("%s%c%s%c%s", user, '\0', user, '\0', pass);
	encoded = g_base64_encode ((unsigned char*) buffer, authlen);
	g_free (buffer);

	return encoded;
}

#ifdef USE_OPENSSL
static char *
str_sha256hash (char *string)
{
	int i;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	char buf[SHA256_DIGEST_LENGTH * 2 + 1];		/* 64 digit hash + '\0' */

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	SHA256 (string, strlen (string), hash);
#else
	SHA256_CTX sha256;

	SHA256_Init (&sha256);
	SHA256_Update (&sha256, string, strlen (string));
	SHA256_Final (hash, &sha256);
#endif

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf (buf + (i * 2), "%02x", hash[i]);
	}

	buf[SHA256_DIGEST_LENGTH * 2] = 0;

	return g_strdup (buf);
}

static char *
rfc_strlower (const char *str)
{
	size_t i, len = strlen(str);
	char *lower = g_new(char, len + 1);

	for (i = 0; i < len; ++i)
	{
		lower[i] = rfc_tolower(str[i]);
	}
	lower[i] = '\0';

	return lower;
}

/**
 * \brief Generate CHALLENGEAUTH response for QuakeNet login.
 *
 * \param username QuakeNet user name
 * \param password password for the user
 * \param challenge the CHALLENGE response we got from Q
 *
 * After a successful connection to QuakeNet a CHALLENGE is requested from Q.
 * Generate the CHALLENGEAUTH response from this CHALLENGE and our user
 * credentials as per the
 * <a href="http://www.quakenet.org/development/challengeauth">CHALLENGEAUTH</a>
 * docs. As for using OpenSSL HMAC, see
 * <a href="http://www.askyb.com/cpp/openssl-hmac-hasing-example-in-cpp/">example 1</a>,
 * <a href="http://stackoverflow.com/questions/242665/understanding-engine-initialization-in-openssl">example 2</a>.
 */
char *
challengeauth_response (const char *username, const char *password, const char *challenge)
{
	int i;
	char *user;
	char *pass;
	char *passhash;
	char *key;
	char *keyhash;
	unsigned char *digest;
	GString *buf = g_string_new_len (NULL, SHA256_DIGEST_LENGTH * 2);

	user = rfc_strlower (username); /* convert username to lowercase as per the RFC */

	pass = g_strndup (password, 10);			/* truncate to 10 characters */
	passhash = str_sha256hash (pass);
	g_free (pass);

	key = g_strdup_printf ("%s:%s", user, passhash);
	g_free (user);
	g_free (passhash);

	keyhash = str_sha256hash (key);
	g_free (key);

	digest = HMAC (EVP_sha256 (), keyhash, strlen (keyhash), (unsigned char *) challenge, strlen (challenge), NULL, NULL);
	g_free (keyhash);

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		g_string_append_printf (buf, "%02x", (unsigned int) digest[i]);
	}

	return g_string_free (buf, FALSE);
}
#endif

/**
 * \brief Wrapper around strftime for Windows
 *
 * Prevents crashing when using an invalid format by escaping them.
 *
 * Behaves the same as strftime with the addition that
 * it returns 0 if the escaped format string is too large.
 *
 * Based upon work from znc-msvc project.
 *
 * This assumes format is a locale-encoded string. For utf-8 strings, use strftime_utf8
 */
size_t
strftime_validated (char *dest, size_t destsize, const char *format, const struct tm *time)
{
#ifndef WIN32
	return strftime (dest, destsize, format, time);
#else
	char safe_format[64];
	const char *p = format;
	int i = 0;

	if (strlen (format) >= sizeof(safe_format))
		return 0;

	memset (safe_format, 0, sizeof(safe_format));

	while (*p)
	{
		if (*p == '%')
		{
			int has_hash = (*(p + 1) == '#');
			char c = *(p + (has_hash ? 2 : 1));

			if (i >= sizeof (safe_format))
				return 0;

			switch (c)
			{
			case 'a': case 'A': case 'b': case 'B': case 'c': case 'd': case 'H': case 'I': case 'j': case 'm': case 'M':
			case 'p': case 'S': case 'U': case 'w': case 'W': case 'x': case 'X': case 'y': case 'Y': case 'z': case 'Z':
			case '%':
				/* formatting code is fine */
				break;
			default:
				/* replace bad formatting code with itself, escaped, e.g. "%V" --> "%%V" */
				g_strlcat (safe_format, "%%", sizeof(safe_format));
				i += 2;
				p++;
				break;
			}

			/* the current loop run will append % (and maybe #) and the next one will do the actual char. */
			if (has_hash)
			{
				safe_format[i] = *p;
				p++;
				i++;
			}
			if (c == '%')
			{
				safe_format[i] = *p;
				p++;
				i++;
			}
		}

		if (*p)
		{
			safe_format[i] = *p;
			p++;
			i++;
		}
	}

	return strftime (dest, destsize, safe_format, time);
#endif
}

/**
 * \brief Similar to strftime except it works with utf-8 formats, since strftime treats the format as locale-encoded.
 */
gsize
strftime_utf8 (char *dest, gsize destsize, const char *format, time_t time)
{
	gsize result;
	GDate* date = g_date_new ();
	g_date_set_time_t (date, time);
	result = g_date_strftime (dest, destsize, format, date);
	g_date_free (date);
	return result;
}
