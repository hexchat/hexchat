#include "dh1080/dh1080.h"
#include "DH1080Wrapper.h"

extern "C" {
	DH1080* newDH1080() {
		return new DH1080();
	}

	const char* DH1080_getNewPublicKey(DH1080* dh) {
		return dh->getNewPublicKey();
	}

	const char* DH1080_computeSymetricKey(DH1080* dh, const char* otherPubKey) {
		return dh->computeSymetricKey(otherPubKey);
	}

	void DH1080_flush(DH1080* dh) {
		return dh->flush();
	}
}
