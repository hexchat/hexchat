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

#include "config.h"

#include <glib.h>
#include "irc.h"

/**
 * Parses an IRC message. The words array should contain the message splitted
 * at spaces. The prefix and command is extracted from the message, and
 * parameters_offset is set to the index of the first parameter.
 */
gboolean irc_parse_message(const char *words[],
                       const char **prefix, const char **command,
                       size_t *parameters_offset) {
    size_t w = 1;
    if (prefix) *prefix = NULL;
    if (command) *command = NULL;
    
    /* See if the message starts with a prefix (sender user) */
    if (words[w][0] == ':') {
        if (prefix) *prefix = &words[w][1];
        w++;
    }
    
    /* Check command */
    if (words[w][0] == '\0') return FALSE;
    if (command) *command = words[w];
    w++;
    
    if (parameters_offset)
		*parameters_offset = w;

    return TRUE;
}


/**
 * Finds the nick part of a "IRC prefix", which can have any
 * of the following forms:
 *
 *     nick
 *     nick@host
 *     nick!ident
 *     nick!ident@host
 */
char *irc_prefix_get_nick(const char *prefix) {
    const char *end;
    size_t length;
    
    if (!prefix) return NULL;
    
    /* Find end of nick */
    end = prefix;
    while (*end != '\0' && *end != '!' && *end != '@') end++;
    
    /* Allocate string */
    length = end - prefix;
    return g_strndup (prefix, length);
}
