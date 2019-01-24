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
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

static jmp_buf reflash_env;

static void
fail(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        longjmp(reflash_env, 1);
}

static void
io_error(void)
{
        fail("Expected reply from device but received none: (%s)\n",
             strerror(errno));
}

static void
check_ok(const char *s)
{
        if (!s)
                io_error();

        if (strncmp(s, "OK", 2) != 0)
                fail("Unexpected result of FLASH WRITE: %s\n", s);
}

static void
generic_flash_write(struct reflash_tcp_t *h, FILE *fp)
{
        char srec[256];
        int lineno = 0;

        fseek(fp, 0, SEEK_SET);
        printf("writing line         ");
        do {
                char *end, *s;

                printf("\033[8D%8d", lineno);
                ++lineno;

                if ((s = fgets(srec, sizeof(srec), fp)) == NULL) {
                        if (feof(fp))
                                break;
                        fclose(fp);
                        fail("\nfgets() returned NULL\n");
                }
                end = &s[strlen(s) - 1];
                while ((*end == '\n' || *end == '\r') && end > s) {
                        *end = '\0';
                        --end;
                }

                check_ok(tcp_io(h, "FLASH WRITE %s", srec));
        } while (!feof(fp));
        putchar('\n');
}

static int
generic_flash_erase(struct reflash_tcp_t *h)
{
        check_ok(tcp_io(h, "FLASH ERASE"));
        return 0;
}

static int
t680_flash_erase(struct reflash_tcp_t *h)
{
        tcp_io_sendonly(h, "FLASH ERASE");
        for (;;) {
                const char *line = tcp_getline(h);
                if (line == NULL)
                        io_error();

                if (strstr(line, "OK")) {
                        break;
                } else if (strstr(line, "31") == NULL)  {
                        fail("Unexpected result of FLASH ERASE: '%s'\n", line);
                }
        }
        return 0;
}

static int
generic_flash_unlock(struct reflash_tcp_t *h)
{
        check_ok(tcp_io(h, "FLASH UNLOCK"));
        return 0;
}

static int
generic_reflash_(struct reflash_tcp_t *h, FILE *fp,
                 int (*ers)(struct reflash_tcp_t *))
{
        if (setjmp(reflash_env) != 0) {
                fprintf(stderr, "Reflash failed\n");
                return -1;
        }

        printf("Unlocking...\n");
        generic_flash_unlock(h);
        printf("Erasing...\n");
        ers(h);
        printf("Reflashing...\n");
        generic_flash_write(h, fp);
        printf("Reflash complete.  Reboot the device for changes to take effect.\n");
        return 0;
}

int
generic_reflash(struct reflash_tcp_t *h, FILE *fp)
{
        return generic_reflash_(h, fp, generic_flash_erase);
}

int
t680_reflash(struct reflash_tcp_t *h, FILE *fp)
{
        return generic_reflash_(h, fp, t680_flash_erase);
}

