//---------------------------------------------------------------------------
//The base64 code used is from 'The Secure Programming Cookbook for C and C++', By John Viega, Matt Messier
// i hit memory fault suggesting improper # of bytes allocated
// i changed from alloc to new/delete to match our expected allocation return freeing
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "b64stuff_static.h"
#include <string.h>
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
//static char b64table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//                           "abcdefghijklmnopqrstuvwxyz"
//                           "0123456789+/";
static char b64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Accepts a binary buffer with an associated size.
 * Returns a base64-encoded, NULL-terminated string.
 */
unsigned char *static_base64_encode(const unsigned char *src, unsigned char* dst, size_t len, int wrap) {
  unsigned char *output, *p;
  size_t        i = 0, mod = len % 3, toalloc;

  // ATTN: WTF is this allocation function - does it fail sometimes?)
  //toalloc = (len / 3) * 4 + (3 - mod) % 3 + 1;
  toalloc = (len / 3) * 4 + (3 - mod) % 3 + 1 + 8;

  if (wrap) {
    toalloc += len / 57;
    if (len % 57) toalloc++;
  }

  if (toalloc < len) return 0;

  //p = output = (unsigned char *)malloc(toalloc);
  p = output = dst ? dst : new unsigned char [toalloc];

  if (!p) return 0;

  while (i < len - mod) {
    *p++ = b64table[src[i++] >> 2];
    *p++ = b64table[((src[i - 1] << 4) | (src[i] >> 4)) & 0x3f];
    *p++ = b64table[((src[i] << 2) | (src[i + 1] >> 6)) & 0x3f];
    *p++ = b64table[src[i + 1] & 0x3f];
    i += 2;
    if (wrap && !(i % 57)) *p++ = '\n';
  }
  if (!mod) {
    if (wrap && i % 57) *p++ = '\n';
    *p = 0;
    return output;
  } else {
    *p++ = b64table[src[i++] >> 2];
    *p++ = b64table[((src[i - 1] << 4) | (src[i] >> 4)) & 0x3f];
    if (mod == 1) {
      *p++ = '=';
      *p++ = '=';
      if (wrap) *p++ = '\n';
      *p = 0;
      return output;
    } else {
      *p++ = b64table[(src[i] << 2) & 0x3f];
      *p++ = '=';
      if (wrap) *p++ = '\n';
      *p = 0;
      return output;
    }
  }
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
static char b64revtb[256] = {
  -3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*0-15*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*16-31*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /*32-47*/
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1, /*48-63*/
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /*64-79*/
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /*80-95*/
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /*96-111*/
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, /*112-127*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*128-143*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*144-159*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*160-175*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*176-191*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*192-207*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*208-223*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*224-239*/
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  /*240-255*/
};

static unsigned int raw_base64_decode(const unsigned char *src, unsigned char *dst,
                                     int strict, int *err) {
  unsigned int  result = 0, x;
  unsigned char buf[3], pad = 0;
  const unsigned char *p = src;

  *err = 0;
  while (!pad) {
    switch ((x = b64revtb[*p++])) {
      case -3: /* NULL TERMINATOR */
        if (((p - 1) - src) % 4) *err = 1;
        return result;
      case -2: /* PADDING CHARACTER. INVALID HERE */
        if (((p - 1) - src) % 4 < 2) {
          *err = 1;
          return result;
        } else if (((p - 1) - src) % 4 == 2) {
          /* Make sure there's appropriate padding */
          if (*p != '=') {
            *err = 1;
            return result;
          }
          buf[2] = 0;
          pad = 2;
          result++;
          break;
        } else {
          pad = 1;
          result += 2;
          break;
        }
        return result;
      case -1:
        if (strict) {
          *err = 2;
          return result;
        }
        break;
      default:
        switch (((p - 1) - src) % 4) {
          case 0:
            buf[0] = x << 2;
            break;
          case 1:
            buf[0] |= (x >> 4);
            buf[1] = x << 4;
            break;
          case 2:
            buf[1] |= (x >> 2);
            buf[2] = x << 6;
            break;
          case 3:
            buf[2] |= x;
            result += 3;
            for (x = 0;  x < 3 - pad;  x++) *dst++ = buf[x];
            break;
        }
        break;
    }
  }
  for (x = 0;  x < 3 - pad;  x++) *dst++ = buf[x];
  return result;
}

/* If err is non-zero on exit, then there was an incorrect padding error.  We
 * allocate enough space for all circumstances, but when there is padding, or
 * there are characters outside the character set in the string (which we are
 * supposed to ignore), then we end up allocating too much space.  You can
 * realloc() to the correct length if you wish.
 */

unsigned char *static_base64_decode(const unsigned char *src, unsigned char *dst, size_t *len, int strict, int *err) {
  unsigned char *outbuf;

//  outbuf = (unsigned char *)malloc(3 * (strlen(buf) / 4 + 1));
  // ATTN: WTF is this allocation function - does it fail sometimes?)
  //outbuf = (unsigned char *)malloc(3 * (strlen((char*)buf) / 4 + 1));
  //outbuf = (unsigned char *)malloc(3 * (strlen((char*)buf)+1));
  //outbuf = new unsigned char [3 * (strlen((char*)buf)+1)];
  outbuf = dst ? dst : new unsigned char [3 * (strlen((char*)src) / 4 + 1 + 8)];
  
  if (!outbuf) {
    *err = -3;
    *len = 0;
    return 0;
  }
  *len = raw_base64_decode(src, outbuf, strict, err);
  if (*err) {
    //free(outbuf);
    if(!dst) delete [] outbuf;
    *len = 0;
    outbuf = 0;
  }
  return outbuf;
}
//---------------------------------------------------------------------------

