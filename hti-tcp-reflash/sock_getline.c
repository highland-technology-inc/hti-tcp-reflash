/*
 * Copyright (c) 2018, Highland Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "reflash.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

struct sock_getline_t {
        char *line;
        size_t len;
        size_t idx;
};

/* Helper to sock_getline.  Add to line, reallocating memory as necessary. */
static int
sock_getline_flush(int c, struct sock_getline_t *buf)
{
        if (buf->line == NULL || buf->idx >= buf->len) {
                char *new = realloc(buf->line, buf->len + 512);
                if (!new)
                        return -1;
                buf->len += 512;
                buf->line = new;
        }
        buf->line[buf->idx] = c;
        buf->idx++;
        return 0;
}

static int
local_getc(FILE *fp)
{
        uint8_t c;
        ssize_t res = read(fileno(fp), &c, 1);
        if (res != 1)
                return EOF;
        return c & 0xffu;
}

ssize_t
sock_getline(char **line, size_t *len, FILE *fp)
{
        static int clast = '\0';

        struct sock_getline_t a;
        int count = 0;
        int c;

        a.line = *line;
        a.len = *len;
        a.idx = 0;

        while ((c = local_getc(fp)) != EOF) {
                /*
                 * Accept CR, LF, or CRLF.
                 * If someone uses the egregiously wrong LFCR
                 * (alas some do), then they're s.o.l.
                 */
                int tmp = clast;
                clast = c;
                switch (c) {
                case '\n':
                        if (tmp == '\r')
                                continue;
                        break;
                case '\r':
                        c = '\n';
                        break;
                default:
                        break;
                }

                ++count;
                if (sock_getline_flush(c, &a) < 0)
                        return (ssize_t)-1;
                if (c == '\n')
                        break;
        }

        /*
         * XXX REVISIT: What if count != 0?
         * Return -1 like getline, or process partial line?
         */
        if (c == EOF)
                return (ssize_t)-1;

        if (sock_getline_flush('\0', &a) < 0)
                return (ssize_t)-1;

        *line = a.line;
        *len = a.len;

        return count;
}

