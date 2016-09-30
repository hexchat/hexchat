#ifndef __DH1080WRAPPER_H
#define __DH1080WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct DH1080 DH1080;
	DH1080* newDH1080();
	const char* DH1080_getNewPublicKey(DH1080* dh);
	const char* DH1080_computeSymetricKey(DH1080* dh, const char* otherPubKey);
	void DH1080_flush(DH1080* dh);

#ifdef __cplusplus
}
#endif
#endif