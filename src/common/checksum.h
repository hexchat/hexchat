#include <openssl/sha.h>
#define BUFSIZE 32768

void sha256_hash_string (unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65]);
void sha256 (char *string, char outputBuffer[65]);
int sha256_file (char *path, char outputBuffer[65]);
