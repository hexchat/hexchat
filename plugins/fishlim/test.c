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
#include <stdio.h>
#include <string.h>
#include "fish.h"

// We can't use the XChat plugin API from here...
gchar *get_config_filename() {
    const gchar *homedir = g_get_home_dir();
    return g_build_filename(homedir, ".xchat2", "blow.ini", NULL);
}


static int decrypt(int nick_count, char *nicks[]) {
    char encrypted[8192];
    while (fgets(encrypted, sizeof(encrypted), stdin)) {
        char *msg;
        for (int i = 0; i < nick_count; i++) {
            msg = fish_decrypt_from_nick(nicks[i], encrypted);
            if (msg) goto success;
        }
        fprintf(stderr, "None of the recipients were found in the key store!\n");
        return 1;
      success:
        fprintf(stderr, "Decrypted text >>>%s<<<\n", msg);
    }
    return 0;
}

static int encrypt(int nick_count, char *nicks[]) {
    char message[8192];
    while (fgets(message, sizeof(message), stdin)) {
        // Remove newline character
        char *newline = strchr(message, '\n');
        if (newline) *newline = '\0';
        
        bool error = false;
        for (int i = 0; i < nick_count; i++) {
            char *encrypted = fish_encrypt_for_nick(nicks[i], message);
            if (encrypted) {
                fprintf(stderr, "Encrypted [%s]:  >>>%s<<<\n", nicks[i], encrypted);
            } else {
                error = true;
            }
        }
        
        if (error) {
            fprintf(stderr, "Some of the recipients were't found in the key store!\n");
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s [-e] nick...\n", argv[0]);
        return 2;
    }
    
    if (strcmp(argv[1], "-e") == 0) {
        return encrypt(argc-2, &argv[2]);
    } else {
        return decrypt(argc-1, &argv[1]);
    }
}


