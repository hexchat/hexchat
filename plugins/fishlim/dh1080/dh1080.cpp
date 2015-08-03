
#include "dh1080.h"
#include "b64stuff_static.h"
#include <openssl/ssl.h>
#include <openssl/sha.h>

#include <string.h>

#define SIZEOFA(x) (sizeof(x)/sizeof(*x))
#define MEMZERO(x) memset(x, 0x00, SIZEOFA(x) );

extern int main();

char DH1080::m_prime1080_hex[] =	"FBE1022E23D213E8ACFA9AE8B9DFADA3"
					"EA6B7AC7A7B7E95AB5EB2DF858921FEA"
					"DE95E6AC7BE7DE6ADBAB8A783E7AF7A7"
					"FA6A2B7BEB1E72EAE2B72F9FA2BFB2A2"
					"EFBEFAC868BADB3E828FA8BADFADA3E4"
					"CC1BE7E8AFE85E9698A783EB68FA07A7"
					"7AB6AD7BEB618ACF9CA2897EB28A6189"
					"EFA07AB99A8A7FA9AE299EFA7BA66DEA"
					"FEFBEFBF0B7D8B";
char DH1080::m_generator_hex[] = 	"2";
int DH1080::m_libInit 			=	0;

DH1080::DH1080()
: m_dh(NULL)
, m_prime1080(NULL)
, m_generator(NULL)
{
	if(!wasLibInit())
		initLib();

	MEMZERO(m_pub_b64);
	MEMZERO(m_sha_b64);

	m_prime1080 = BN_new();
	BN_init(m_prime1080);
	BN_hex2bn(&m_prime1080,m_prime1080_hex);

	m_generator = BN_new();
	BN_init(m_generator);
	BN_hex2bn(&m_generator,m_generator_hex);

	m_dh = DH_new();
	m_dh->p = m_prime1080;
	m_dh->g = m_generator;

	/*
	// unless something is really broken
	// using the given prime and generator pair, nothing should happen
	int chk=0;
	DH_check(m_dh,&chk);
	if(chk!=0)
		throw "b0nk";
	*/
}

DH1080::~DH1080()
{
	if(m_dh)
	{
		m_dh->p = m_dh->g = NULL;
		DH_free(m_dh);
		m_dh = NULL;
	}

	if(m_prime1080)
	{
		BN_free(m_prime1080);
		m_prime1080 = NULL;
	}
	
	if(m_generator)
	{
		BN_free(m_generator);
		m_generator = NULL;
	}

	MEMZERO(m_pub_b64);
	MEMZERO(m_sha_b64);
}

void DH1080::initLib()
{
	// TODO: add return value checks
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	m_libInit = 1;
}

inline bool DH1080::wasLibInit()
{
	return m_libInit==1;
}




const char * DH1080::getNewPublicKey()
{
	unsigned char bufbin[BN_LEN+1];

	// let's start over
	MEMZERO(m_pub_b64);
	MEMZERO(m_sha_b64);
	MEMZERO(bufbin);

	// clean up a bit, we want to create a fresh private/public
	if(m_dh->pub_key) BN_free(m_dh->pub_key);
	m_dh->pub_key = NULL;

	BN_free(m_dh->priv_key);
	if(m_dh->priv_key) m_dh->priv_key = NULL;

	// create keypair
	DH_generate_key(m_dh);

	// convert it to something we can send out
	BN_bn2bin(m_dh->pub_key,bufbin);
	b64enc(bufbin,m_pub_b64, BN_LEN);

	m_pub_b64[PUB_B64_LEN-1]='A';
	m_pub_b64[PUB_B64_LEN]='\0';

	// no longer needed
	MEMZERO(bufbin);
	
	return (const char*)m_pub_b64;
}

const char * DH1080::computeSymetricKey(const char* otherPubKey)
{
	unsigned char bufbin[BN_LEN+1];
	unsigned char bufsha[SHA_LEN+1];
	unsigned char bufpub[PUB_B64_LEN+1];

	BIGNUM *bn = NULL;

	// we're gonna need those
	MEMZERO(m_sha_b64);
	MEMZERO(bufbin);
	MEMZERO(bufsha);
	MEMZERO(bufpub);

	// create a local copy and kill off the "A"
	// that's sticking to the end
	strncpy((char*)bufpub,otherPubKey,PUB_B64_LEN+1);
	bufpub[PUB_B64_LEN-1] = '\0';

	b64dec(bufpub,bufbin);
	bn = BN_bin2bn(bufbin,BN_LEN,NULL);

	DH_compute_key(bufbin,bn,m_dh);
	
	BN_zero(bn);
	BN_free(bn);
	
	// TODO: check if bufsecret already is a BIGNUM-BIN
	// we're just assuming it is
	// NOTE: lucky! it works!
	sha256(bufbin,bufsha);

	b64enc(bufsha, m_sha_b64, SHA_LEN);

	// no longer needed
	MEMZERO(bufbin);
	MEMZERO(bufsha);

	if(m_dh->pub_key) BN_free(m_dh->pub_key);
	m_dh->pub_key = NULL;

	BN_free(m_dh->priv_key);
	if(m_dh->priv_key) m_dh->priv_key = NULL;

	// kill the appended "=", it's not used
	m_sha_b64[43] = '\0';

	return (const char*)m_sha_b64;
}

void DH1080::b64enc(const unsigned char* src, unsigned char* dst, size_t len)
{
	// everything that happens down here is based on static buffers
	// that already have the correct size (*cough*)
	// 
	static_base64_encode(src, dst, len, 0);
}

void DH1080::b64dec(const unsigned char* src, unsigned char* dst)
{
	// everything that happens down here is based on static buffers
	// that already have the correct size (*cough*)
	// 
	size_t len=0;
	int err=0;
	static_base64_decode(src, dst, &len, 0, &err);
}

void DH1080::sha256(const unsigned char* src, unsigned char* dst)
{
	// we are always encoding the BIN!
	SHA256(src,BN_LEN,dst);
}

void DH1080::flush()
{
	MEMZERO(m_pub_b64);
	MEMZERO(m_sha_b64);

	if(m_dh->pub_key) BN_free(m_dh->pub_key);
	m_dh->pub_key = NULL;

	BN_free(m_dh->priv_key);
	if(m_dh->priv_key) m_dh->priv_key = NULL;
}

