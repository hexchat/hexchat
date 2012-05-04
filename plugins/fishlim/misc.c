/*

  Copyright (c) 2010 Samuel Lidén Borell <samuel@kodafritt.se>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

*/

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"


void secure_erase(void *ptr, size_t size) {
    // "volatile" prevents this code from being optimized away
    volatile char* volptr = ptr;
    while (size--) *volptr++ = 0;
}

/**
 * Re-allocates a string with the native allocator.
 */
char *import_glib_string(gchar *gstr) {
    size_t size;
    char *native;
    if (g_mem_is_system_malloc()) return gstr;
    
    size = strlen(gstr)+1;
    native = malloc(size);
    memcpy(native, gstr, size);
    
    secure_erase(gstr, size);
    g_free(gstr);
    return native;
}


