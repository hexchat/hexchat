/*
 * ssl.c v0.0.3
 * Copyright (C) 2000  --  DaP <profeta@freemail.c3.hu>
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

#ifdef __APPLE__
#define __AVAILABILITYMACROS__
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_7_AND_LATER
#endif

#include "inet.h"				  /* make it first to avoid macro redefinitions */
#include <openssl/ssl.h>		  /* SSL_() */
#include <openssl/err.h>		  /* ERR_() */
#include <openssl/x509v3.h>
#ifdef WIN32
#include <openssl/rand.h>		  /* RAND_seed() */
#endif
#include "config.h"
#include <time.h>				  /* asctime() */
#include <string.h>				  /* strncpy() */
#include "ssl.h"				  /* struct cert_info */

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include "util.h"

/* If openssl was built without ec */
#ifndef SSL_OP_SINGLE_ECDH_USE
#define SSL_OP_SINGLE_ECDH_USE 0
#endif

#ifndef SSL_OP_NO_COMPRESSION
#define SSL_OP_NO_COMPRESSION 0
#endif

/* globals */
static struct chiper_info chiper_info;		/* static buffer for _SSL_get_cipher_info() */
static char err_buf[256];			/* generic error buffer */


/* +++++ Internal functions +++++ */

static void
__SSL_fill_err_buf (char *funcname)
{
	int err;
	char buf[256];


	err = ERR_get_error ();
	ERR_error_string (err, buf);
	g_snprintf (err_buf, sizeof (err_buf), "%s: %s (%d)\n", funcname, buf, err);
}


static void
__SSL_critical_error (char *funcname)
{
	__SSL_fill_err_buf (funcname);
	fprintf (stderr, "%s\n", err_buf);

	exit (1);
}

/* +++++ SSL functions +++++ */

SSL_CTX *
_SSL_context_init (void (*info_cb_func))
{
	SSL_CTX *ctx;

	SSLeay_add_ssl_algorithms ();
	SSL_load_error_strings ();
	ctx = SSL_CTX_new (SSLv23_client_method ());

	SSL_CTX_set_session_cache_mode (ctx, SSL_SESS_CACHE_BOTH);
	SSL_CTX_set_timeout (ctx, 300);
	SSL_CTX_set_options (ctx, SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3
							  |SSL_OP_NO_COMPRESSION
							  |SSL_OP_SINGLE_DH_USE|SSL_OP_SINGLE_ECDH_USE
							  |SSL_OP_NO_TICKET
							  |SSL_OP_CIPHER_SERVER_PREFERENCE);

#if OPENSSL_VERSION_NUMBER >= 0x00908000L && !defined (OPENSSL_NO_COMP) /* workaround for OpenSSL 0.9.8 */
	sk_SSL_COMP_zero(SSL_COMP_get_compression_methods());
#endif

	/* used in SSL_connect(), SSL_accept() */
	SSL_CTX_set_info_callback (ctx, info_cb_func);

	return(ctx);
}

static void
ASN1_TIME_snprintf (char *buf, int buf_len, ASN1_TIME * tm)
{
	char *expires = NULL;
	BIO *inMem = BIO_new (BIO_s_mem ());

	ASN1_TIME_print (inMem, tm);
	BIO_get_mem_data (inMem, &expires);
	buf[0] = 0;
	if (expires != NULL)
	{
		/* expires is not \0 terminated */
		safe_strcpy (buf, expires, MIN(24, buf_len));
	}
	BIO_free (inMem);
}


static void
broke_oneline (char *oneline, char *parray[])
{
	char *pt, *ppt;
	int i;


	i = 0;
	ppt = pt = oneline + 1;
	while ((pt = strchr (pt, '/')))
	{
		*pt = 0;
		parray[i++] = ppt;
		ppt = ++pt;
	}
	parray[i++] = ppt;
	parray[i] = NULL;
}


/*
    FIXME: Master-Key, Extensions, CA bits
	    (openssl x509 -text -in servcert.pem)
*/
int
_SSL_get_cert_info (struct cert_info *cert_info, SSL * ssl)
{
	X509 *peer_cert;
	X509_PUBKEY *key;
	X509_ALGOR *algor = NULL;
	EVP_PKEY *peer_pkey;
	char notBefore[64];
	char notAfter[64];
	int alg;
	int sign_alg;


	if (!(peer_cert = SSL_get_peer_certificate (ssl)))
		return (1);				  /* FATAL? */

	X509_NAME_oneline (X509_get_subject_name (peer_cert), cert_info->subject,
							 sizeof (cert_info->subject));
	X509_NAME_oneline (X509_get_issuer_name (peer_cert), cert_info->issuer,
							 sizeof (cert_info->issuer));
	broke_oneline (cert_info->subject, cert_info->subject_word);
	broke_oneline (cert_info->issuer, cert_info->issuer_word);

	key = X509_get_X509_PUBKEY(peer_cert);
	if (!X509_PUBKEY_get0_param(NULL, NULL, 0, &algor, key))
		return 1;

	alg = OBJ_obj2nid (algor->algorithm);
#ifndef HAVE_X509_GET_SIGNATURE_NID
	sign_alg = OBJ_obj2nid (peer_cert->sig_alg->algorithm);
#else
	sign_alg = X509_get_signature_nid (peer_cert);
#endif
	ASN1_TIME_snprintf (notBefore, sizeof (notBefore),
							  X509_get_notBefore (peer_cert));
	ASN1_TIME_snprintf (notAfter, sizeof (notAfter),
							  X509_get_notAfter (peer_cert));

	peer_pkey = X509_get_pubkey (peer_cert);

	safe_strcpy (cert_info->algorithm,
				(alg == NID_undef) ? "Unknown" : OBJ_nid2ln (alg),
				sizeof (cert_info->algorithm));
	cert_info->algorithm_bits = EVP_PKEY_bits (peer_pkey);
	safe_strcpy (cert_info->sign_algorithm,
				(sign_alg == NID_undef) ? "Unknown" : OBJ_nid2ln (sign_alg),
				sizeof (cert_info->sign_algorithm));
	/* EVP_PKEY_bits(ca_pkey)); */
	cert_info->sign_algorithm_bits = 0;
	safe_strcpy (cert_info->notbefore, notBefore, sizeof (cert_info->notbefore));
	safe_strcpy (cert_info->notafter, notAfter, sizeof (cert_info->notafter));

	EVP_PKEY_free (peer_pkey);

	/* SSL_SESSION_print_fp(stdout, SSL_get_session(ssl)); */
/*
	if (ssl->session->sess_cert->peer_rsa_tmp) {
		tmp_pkey = EVP_PKEY_new();
		EVP_PKEY_assign_RSA(tmp_pkey, ssl->session->sess_cert->peer_rsa_tmp);
		cert_info->rsa_tmp_bits = EVP_PKEY_bits (tmp_pkey);
		EVP_PKEY_free(tmp_pkey);
	} else
		fprintf(stderr, "REMOTE SIDE DOESN'T PROVIDES ->peer_rsa_tmp\n");
*/
	cert_info->rsa_tmp_bits = 0;

	X509_free (peer_cert);

	return (0);
}


struct chiper_info *
_SSL_get_cipher_info (SSL * ssl)
{
	const SSL_CIPHER *c;


	c = SSL_get_current_cipher (ssl);
	safe_strcpy (chiper_info.version, SSL_CIPHER_get_version (c),
				sizeof (chiper_info.version));
	safe_strcpy (chiper_info.chiper, SSL_CIPHER_get_name (c),
				sizeof (chiper_info.chiper));
	SSL_CIPHER_get_bits (c, &chiper_info.chiper_bits);

	return (&chiper_info);
}


int
_SSL_send (SSL * ssl, char *buf, int len)
{
	int num;


	num = SSL_write (ssl, buf, len);

	switch (SSL_get_error (ssl, num))
	{
	case SSL_ERROR_SSL:			  /* setup errno! */
		/* ??? */
		__SSL_fill_err_buf ("SSL_write");
		fprintf (stderr, "%s\n", err_buf);
		break;
	case SSL_ERROR_SYSCALL:
		/* ??? */
		perror ("SSL_write/write");
		break;
	case SSL_ERROR_ZERO_RETURN:
		/* fprintf(stderr, "SSL closed on write\n"); */
		break;
	}

	return (num);
}


int
_SSL_recv (SSL * ssl, char *buf, int len)
{
	int num;


	num = SSL_read (ssl, buf, len);

	switch (SSL_get_error (ssl, num))
	{
	case SSL_ERROR_SSL:
		/* ??? */
		__SSL_fill_err_buf ("SSL_read");
		fprintf (stderr, "%s\n", err_buf);
		break;
	case SSL_ERROR_SYSCALL:
		/* ??? */
		if (!would_block ())
			perror ("SSL_read/read");
		break;
	case SSL_ERROR_ZERO_RETURN:
		/* fprintf(stdeerr, "SSL closed on read\n"); */
		break;
	}

	return (num);
}


SSL *
_SSL_socket (SSL_CTX *ctx, int sd)
{
	SSL *ssl;
	const SSL_METHOD *method;

	if (!(ssl = SSL_new (ctx)))
		/* FATAL */
		__SSL_critical_error ("SSL_new");

	SSL_set_fd (ssl, sd);

#ifndef HAVE_SSL_CTX_GET_SSL_METHOD
	method = ctx->method;
#else
	method = SSL_CTX_get_ssl_method (ctx);
#endif
	if (method == SSLv23_client_method())
		SSL_set_connect_state (ssl);
	else
	        SSL_set_accept_state(ssl);

	return (ssl);
}


char *
_SSL_set_verify (SSL_CTX *ctx, void *verify_callback)
{
#ifdef DEFAULT_CERT_FILE
	if (!SSL_CTX_load_verify_locations (ctx, DEFAULT_CERT_FILE, NULL))
	{
		__SSL_fill_err_buf ("SSL_CTX_load_verify_locations");
		return (err_buf);
	}
#else
	if (!SSL_CTX_set_default_verify_paths (ctx))
	{
		__SSL_fill_err_buf ("SSL_CTX_set_default_verify_paths");
		return (err_buf);
	}
#endif

	SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, verify_callback);

	return (NULL);
}


void
_SSL_close (SSL * ssl)
{
	SSL_set_shutdown (ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
	SSL_free (ssl);
#ifdef HAVE_ERR_REMOVE_THREAD_STATE
#if OPENSSL_VERSION_NUMBER >= 0x10000000L && OPENSSL_VERSION_NUMBER < 0x10100000L
	/* OpenSSL handles this itself in 1.1+ and this is a no-op */
	ERR_remove_thread_state (NULL);
#endif
#else
	ERR_remove_state (0);
#endif
}

/* Hostname validation code based on OpenBSD's libtls. */

static int
_SSL_match_hostname (const char *cert_hostname, const char *hostname)
{
	const char *cert_domain, *domain, *next_dot;

	if (g_ascii_strcasecmp (cert_hostname, hostname) == 0)
		return 0;

	/* Wildcard match? */
	if (cert_hostname[0] == '*')
	{
		/*
		 * Valid wildcards:
		 * - "*.domain.tld"
		 * - "*.sub.domain.tld"
		 * - etc.
		 * Reject "*.tld".
		 * No attempt to prevent the use of eg. "*.co.uk".
		 */
		cert_domain = &cert_hostname[1];
		/* Disallow "*"  */
		if (cert_domain[0] == '\0')
			return -1;
		/* Disallow "*foo" */
		if (cert_domain[0] != '.')
			return -1;
		/* Disallow "*.." */
		if (cert_domain[1] == '.')
			return -1;
		next_dot = strchr (&cert_domain[1], '.');
		/* Disallow "*.bar" */
		if (next_dot == NULL)
			return -1;
		/* Disallow "*.bar.." */
		if (next_dot[1] == '.')
			return -1;

		domain = strchr (hostname, '.');

		/* No wildcard match against a hostname with no domain part. */
		if (domain == NULL || strlen(domain) == 1)
			return -1;

		if (g_ascii_strcasecmp (cert_domain, domain) == 0)
			return 0;
	}

	return -1;
}

static int
_SSL_check_subject_altname (X509 *cert, const char *host)
{
	STACK_OF(GENERAL_NAME) *altname_stack = NULL;
	GInetAddress *addr;
	GSocketFamily family;
	int type = GEN_DNS;
	int count, i;
	int rv = -1;

	altname_stack = X509_get_ext_d2i (cert, NID_subject_alt_name, NULL, NULL);
	if (altname_stack == NULL)
		return -1;

	addr = g_inet_address_new_from_string (host);
	if (addr != NULL)
	{
		family = g_inet_address_get_family (addr);
		if (family == G_SOCKET_FAMILY_IPV4 || family == G_SOCKET_FAMILY_IPV6)
			type = GEN_IPADD;
	}

	count = sk_GENERAL_NAME_num(altname_stack);
	for (i = 0; i < count; i++)
	{
		GENERAL_NAME *altname;

		altname = sk_GENERAL_NAME_value (altname_stack, i);

		if (altname->type != type)
			continue;

		if (type == GEN_DNS)
		{
			const unsigned char *data;
			int format;

			format = ASN1_STRING_type (altname->d.dNSName);
			if (format == V_ASN1_IA5STRING)
			{
#ifdef HAVE_ASN1_STRING_GET0_DATA
				data = ASN1_STRING_get0_data (altname->d.dNSName);
#else
				data = ASN1_STRING_data (altname->d.dNSName);
#endif

				if (ASN1_STRING_length (altname->d.dNSName) != (int)strlen(data))
				{
					g_warning("NUL byte in subjectAltName, probably a malicious certificate.\n");
					rv = -2;
					break;
				}

				if (_SSL_match_hostname (data, host) == 0)
				{
					rv = 0;
					break;
				}
			}
			else
				g_warning ("unhandled subjectAltName dNSName encoding (%d)\n", format);

		}
		else if (type == GEN_IPADD)
		{
			const unsigned char *data;
			const guint8 *addr_bytes;
			int datalen, addr_len;

			datalen = ASN1_STRING_length (altname->d.iPAddress);
#ifdef HAVE_ASN1_STRING_GET0_DATA
			data = ASN1_STRING_get0_data (altname->d.iPAddress);
#else
			data = ASN1_STRING_data (altname->d.iPAddress);
#endif

			addr_bytes = g_inet_address_to_bytes (addr);
			addr_len = (int)g_inet_address_get_native_size (addr);

			if (datalen == addr_len && memcmp (data, addr_bytes, addr_len) == 0)
			{
				rv = 0;
				break;
			}
		}
	}

	if (addr != NULL)
		g_object_unref (addr);
	sk_GENERAL_NAME_pop_free (altname_stack, GENERAL_NAME_free);
	return rv;
}

static int
_SSL_check_common_name (X509 *cert, const char *host)
{
	X509_NAME *name;
	char *common_name = NULL;
	int common_name_len;
	int rv = -1;
	GInetAddress *addr;

	name = X509_get_subject_name (cert);
	if (name == NULL)
		return -1;

	common_name_len = X509_NAME_get_text_by_NID (name, NID_commonName, NULL, 0);
	if (common_name_len < 0)
		return -1;

	common_name = g_malloc0 (common_name_len + 1);

	X509_NAME_get_text_by_NID (name, NID_commonName, common_name, common_name_len + 1);

	/* NUL bytes in CN? */
	if (common_name_len != (int)strlen(common_name))
	{
		g_warning ("NUL byte in Common Name field, probably a malicious certificate.\n");
		rv = -2;
		goto out;
	}

	if ((addr = g_inet_address_new_from_string (host)) != NULL)
	{
		/*
		 * We don't want to attempt wildcard matching against IP
		 * addresses, so perform a simple comparison here.
		 */
		if (g_strcmp0 (common_name, host) == 0)
			rv = 0;
		else
			rv = -1;

		g_object_unref (addr);
	}
	else if (_SSL_match_hostname (common_name, host) == 0)
		rv = 0;

out:
	g_free(common_name);
	return rv;
}

int
_SSL_check_hostname (X509 *cert, const char *host)
{
	int rv;

	rv = _SSL_check_subject_altname (cert, host);
	if (rv == 0 || rv == -2)
		return rv;

	return _SSL_check_common_name (cert, host);
}
