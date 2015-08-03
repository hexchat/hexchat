
#ifndef _dh1080_h_
#define _dh1080_h_

#include <openssl/dh.h>
#include <openssl/bn.h>



class DH1080
{
	/// check if the openssl library was already initialized
	/// this only happens once!
	static int m_libInit;

	/// the length of a BIGNUM or BIN
	/// all big integers used are 135 bytes aka 1080 bits
	static const unsigned int BN_LEN = 135;

	/// the length of a base64 encoded public key
	/// base64 coding increases the string lengtsh by N * 4/3 + 2*N%3
	/// this happens to be 180 bytes
	/// fish appends an 'A' to all base64 coded strings
	static const unsigned int PUB_B64_LEN = 181;

	/// the length of a SHA256 hash
	/// 256 bit = 32 byte
	static const unsigned int SHA_LEN = 32;

	/// the length of the shared secret
	/// the computed shared secret BIGNUM converted to BIN is then
	/// SHA256 hashed (256 bits / 32 bytes)
	/// the hash is then base64 encoded resulting in (32*4/3)+(2*32%3) = 45 bytes
	static const unsigned int SHA_B64_LEN = 45;

	/// a buffer size that's a bit bigger than the biggest expected string
	static const unsigned int SAFE_BUFFER = PUB_B64_LEN + 32;
	
	/// this holds the hardcoded sophie-germain prime (1080bits)
	static char m_prime1080_hex[];

	/// this holds the hardcoded generator (2)
	static char m_generator_hex[];

	/// buffer for our public key
	unsigned char m_pub_b64[PUB_B64_LEN+1];

	/// buffer for generated symetric encryption key
	unsigned char m_sha_b64[SHA_B64_LEN+1];

	/// openssl DH context
	DH *m_dh;

	/// the prime in BN format
	BIGNUM *m_prime1080;

	/// the generator in BN format
	BIGNUM *m_generator;

	void b64enc(const unsigned char* src, unsigned char* dst, size_t len);
	void b64dec(const unsigned char* src, unsigned char* dst);
	void sha256(const unsigned char* src, unsigned char* dst);
	bool wasLibInit();
	void initLib();

	DH1080(const DH1080&);
	DH1080& operator=(const DH1080&);
public:
	DH1080();
	virtual ~DH1080();

	const char * getNewPublicKey();
	const char * computeSymetricKey(const char* otherPubKey);
	void flush();
};

#endif //_dh1080_h_
