//---------------------------------------------------------------------------
//The base64 code used is from 'The Secure Programming Cookbook for C and C++', By John Viega, Matt Messier
// option for static buffers added by Gregor Jehle <gjehle@gmail.com>
//---------------------------------------------------------------------------

#ifndef _b64stuffh
#define _b64stuffh

#include <stdlib.h>

unsigned char *static_base64_encode(const unsigned char *src, unsigned char *dst, size_t len, int wrap);
unsigned char *static_base64_decode(const unsigned char *src, unsigned char *dst, size_t *len, int strict, int *err);
#endif
