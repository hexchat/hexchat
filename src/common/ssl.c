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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <openssl/ssl.h>		  /* SSL_() */
#include <openssl/err.h>		  /* ERR_() */
#include <time.h>					  /* asctime() */
#include <string.h>				  /* strncpy() */
#include "ssl.h"					  /* struct cert_info */
#include "inet.h"
#include "../../config.h"		  /* HAVE_SNPRINTF */

#ifndef HAVE_SNPRINTF
#define snprintf g_snprintf
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
	snprintf (err_buf, sizeof (err_buf), "%s: %s (%d)\n", funcname, buf, err);
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
_SSL_context_init (void (*info_cb_func), int server)
{
	SSL_CTX *ctx;
#ifdef WIN32
	int i, r;
#endif

	SSLeay_add_ssl_algorithms ();
	SSL_load_error_strings ();
	ctx = SSL_CTX_new (server ? SSLv3_server_method() : SSLv3_client_method ());

	SSL_CTX_set_session_cache_mode (ctx, SSL_SESS_CACHE_BOTH);
	SSL_CTX_set_timeout (ctx, 300);

	/* used in SSL_connect(), SSL_accept() */
	SSL_CTX_set_info_callback (ctx, info_cb_func);

#ifdef WIN32
	/* under win32, OpenSSL needs to be seeded with some randomness */
	for (i = 0; i < 128; i++)
	{
		r = rand ();
		RAND_seed ((unsigned char *)&r, sizeof (r));
	}
#endif

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
		memset (buf, 0, buf_len);
		strncpy (buf, expires, 24);
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
	EVP_PKEY *peer_pkey;
	/* EVP_PKEY *ca_pkey; */
	/* EVP_PKEY *tmp_pkey; */
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

	alg = OBJ_obj2nid (peer_cert->cert_info->key->algor->algorithm);
	sign_alg = OBJ_obj2nid (peer_cert->sig_alg->algorithm);
	ASN1_TIME_snprintf (notBefore, sizeof (notBefore),
							  X509_get_notBefore (peer_cert));
	ASN1_TIME_snprintf (notAfter, sizeof (notAfter),
							  X509_get_notAfter (peer_cert));

	peer_pkey = X509_get_pubkey (peer_cert);

	strncpy (cert_info->algorithm,
				(alg == NID_undef) ? "Unknown" : OBJ_nid2ln (alg),
				sizeof (cert_info->algorithm));
	cert_info->algorithm_bits = EVP_PKEY_bits (peer_pkey);
	strncpy (cert_info->sign_algorithm,
				(sign_alg == NID_undef) ? "Unknown" : OBJ_nid2ln (sign_alg),
				sizeof (cert_info->sign_algorithm));
	/* EVP_PKEY_bits(ca_pkey)); */
	cert_info->sign_algorithm_bits = 0;
	strncpy (cert_info->notbefore, notBefore, sizeof (cert_info->notbefore));
	strncpy (cert_info->notafter, notAfter, sizeof (cert_info->notafter));

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

	X509_free (peer_cert);

	return (0);
}


struct chiper_info *
_SSL_get_cipher_info (SSL * ssl)
{
	SSL_CIPHER *c;


	c = SSL_get_current_cipher (ssl);
	strncpy (chiper_info.version, SSL_CIPHER_get_version (c),
				sizeof (chiper_info.version));
	strncpy (chiper_info.chiper, SSL_CIPHER_get_name (c),
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


	if (!(ssl = SSL_new (ctx)))
		/* FATAL */
		__SSL_critical_error ("SSL_new");

	SSL_set_fd (ssl, sd);
	if (ctx->method == SSLv3_client_method())
		SSL_set_connect_state (ssl);
	else
	        SSL_set_accept_state(ssl);

	return (ssl);
}


char *
_SSL_set_verify (SSL_CTX *ctx, void *verify_callback, char *cacert)
{
	if (!SSL_CTX_set_default_verify_paths (ctx))
	{
		__SSL_fill_err_buf ("SSL_CTX_set_default_verify_paths");
		return (err_buf);
	}
/*
	if (cacert)
	{
		if (!SSL_CTX_load_verify_locations (ctx, cacert, NULL))
		{
			__SSL_fill_err_buf ("SSL_CTX_load_verify_locations");
			return (err_buf);
		}
	}
*/
	SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, verify_callback);

	return (NULL);
}


void
_SSL_close (SSL * ssl)
{
	SSL_set_shutdown (ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
	SSL_free (ssl);
	ERR_remove_state (0);		  /* free state buffer */
}
