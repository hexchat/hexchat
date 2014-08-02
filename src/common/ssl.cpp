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
#ifdef WIN32
#include <openssl/rand.h>		  /* RAND_seed() */
#endif
#include "../../config.h"
#include <string>
#include <iterator>
#include <iostream>
#include <sstream>
#include <memory>
#include <algorithm>
#include <ctime>				  /* asctime() */
#include <cstring>				  /* strncpy() */
#include "ssl.hpp"				  /* struct cert_info */

/* globals */
static struct chiper_info chiper_info;		/* static buffer for _SSL_get_cipher_info() */
static char err_buf[256];			/* generic error buffer */


/* +++++ Internal functions +++++ */
namespace {
static std::string
__SSL_fill_err_buf (const std::string & funcname)
{
	char buf[256] = { 0 };
	unsigned long err = ERR_get_error ();
	ERR_error_string_n (err, buf, sizeof(buf));

	std::ostringstream stream(funcname);
	stream << ": " << buf << " (" << err << ")\n";
	auto err_string = stream.str();
	std::copy(err_string.cbegin(), err_string.cend(), std::begin(err_buf));
	return err_string;
}

static void
__SSL_critical_error (const std::string & funcname)
{
	std::cerr << __SSL_fill_err_buf(funcname) << std::endl;

	std::exit (1);
}
}

/* +++++ SSL functions +++++ */

SSL_CTX *
_SSL_context_init (void (*info_cb_func), int server)
{
	SSL_CTX *ctx;

	SSLeay_add_ssl_algorithms ();
	SSL_load_error_strings ();
	ctx = SSL_CTX_new (server ? SSLv23_server_method() : SSLv23_client_method ());

	SSL_CTX_set_session_cache_mode (ctx, SSL_SESS_CACHE_BOTH);
	SSL_CTX_set_timeout (ctx, 300);

	/* used in SSL_connect(), SSL_accept() */
	SSL_CTX_set_info_callback(ctx, (void(*)(const SSL *, int, int)) info_cb_func);

#ifdef WIN32
	/* under win32, OpenSSL needs to be seeded with some randomness */
	for (int i = 0; i < 128; i++)
	{
		int r = rand ();
		RAND_seed ((unsigned char *)&r, sizeof (r));
	}
#endif

	return(ctx);
}
namespace {

auto bio_deleter = [](BIO* d){ BIO_free(d); };
static std::string
ASN1_TIME_to_string (ASN1_TIME * tm)
{
	char *expires = nullptr;
	std::unique_ptr<BIO, decltype(bio_deleter)> inMem(BIO_new (BIO_s_mem ()), bio_deleter);

	ASN1_TIME_print (inMem.get(), tm);
	BIO_get_mem_data (inMem.get(), &expires);
	std::string buf;
	if (expires)
	{
		buf.append(expires, 24);
	}
	return buf;
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
}

/*
    FIXME: Master-Key, Extensions, CA bits
	    (openssl x509 -text -in servcert.pem)
*/
int
_SSL_get_cert_info (cert_info &cert_info, SSL * ssl)
{
	X509 *peer_cert;
	EVP_PKEY *peer_pkey;
	/* EVP_PKEY *ca_pkey; */
	/* EVP_PKEY *tmp_pkey; */
	int alg;
	int sign_alg;


	if (!(peer_cert = SSL_get_peer_certificate (ssl)))
		return (1);				  /* FATAL? */

	X509_NAME_oneline (X509_get_subject_name (peer_cert), cert_info.subject,
							 sizeof (cert_info.subject));
	X509_NAME_oneline (X509_get_issuer_name (peer_cert), cert_info.issuer,
							 sizeof (cert_info.issuer));
	broke_oneline (cert_info.subject, cert_info.subject_word);
	broke_oneline (cert_info.issuer, cert_info.issuer_word);

	alg = OBJ_obj2nid (peer_cert->cert_info->key->algor->algorithm);
	sign_alg = OBJ_obj2nid (peer_cert->sig_alg->algorithm);
	auto notBefore = ASN1_TIME_to_string (X509_get_notBefore (peer_cert));
	auto notAfter = ASN1_TIME_to_string (X509_get_notAfter (peer_cert));

	peer_pkey = X509_get_pubkey (peer_cert);

	strncpy (cert_info.algorithm,
				(alg == NID_undef) ? "Unknown" : OBJ_nid2ln (alg),
				sizeof (cert_info.algorithm));
	cert_info.algorithm_bits = EVP_PKEY_bits (peer_pkey);
	strncpy (cert_info.sign_algorithm,
				(sign_alg == NID_undef) ? "Unknown" : OBJ_nid2ln (sign_alg),
				sizeof (cert_info.sign_algorithm));
	/* EVP_PKEY_bits(ca_pkey)); */
	cert_info.sign_algorithm_bits = 0;
	std::copy(notBefore.cbegin(), notBefore.cend(), std::begin(cert_info.notbefore));
	std::copy(notAfter.cbegin(), notAfter.cend(), std::begin(cert_info.notafter));

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
	cert_info.rsa_tmp_bits = 0;

	X509_free (peer_cert);

	return (0);
}


struct chiper_info *
_SSL_get_cipher_info (SSL * ssl)
{
	const SSL_CIPHER *c;


	c = SSL_get_current_cipher (ssl);
	strncpy (chiper_info.version, SSL_CIPHER_get_version (c),
				sizeof (chiper_info.version));
	strncpy (chiper_info.chiper, SSL_CIPHER_get_name (c),
				sizeof (chiper_info.chiper));
	SSL_CIPHER_get_bits (c, &chiper_info.chiper_bits);

	return &chiper_info;
}


int
_SSL_send (SSL * ssl, const char *buf, int len)
{
	int num;


	num = SSL_write (ssl, buf, len);

	switch (SSL_get_error (ssl, num))
	{
	case SSL_ERROR_SSL:			  /* setup errno! */
		/* ??? */
		std::cerr << __SSL_fill_err_buf("SSL_write") << std::endl;
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
		std::cerr << __SSL_fill_err_buf("SSL_read") << std::endl;
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
	if (ctx->method == SSLv23_client_method())
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
	SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, (int(*)(int, X509_STORE_CTX*))verify_callback);

	return (NULL);
}


void
_SSL_close (SSL * ssl)
{
	SSL_set_shutdown (ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
	SSL_free (ssl);
	ERR_remove_state (0);		  /* free state buffer */
}
