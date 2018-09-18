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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void
check_ok(const char *s)
{
        if (strncmp(s, "OK", 2)) {
            fprintf(stderr, "Unexpected result of FLASH WRITE: %s\n", s);
            exit(1);
        }
}

static int
generic_flash_write(struct REFLASH_TCP_T *h, FILE *fp)
{
        char srec[256];
        char outbuf[256];
        char inbuf[256];
        int lineno = 0;
        fseek(fp, 0, SEEK_SET);
        printf("writing line         ");
        do {
                printf("\033[8D%8d", lineno);
                ++lineno;

                char *end;
                char *s = fgets(srec, sizeof(srec), fp);
                if (s == NULL) {
                        if (feof(fp))
                                break;
                        fprintf(stderr, "fgets() returned NULL\n");
                        goto err;
                }
                end = &s[strlen(s) - 1];
                while ((*end == '\n' || *end == '\r') && end > s) {
                        *end = '\0';
                        --end;
                }

                snprintf(outbuf, sizeof(outbuf), "FLASH WRITE %s\r", srec);

                tcp_io(h, outbuf, inbuf, sizeof(inbuf));
                check_ok(inbuf);
        } while (!feof(fp));
        putchar('\n');
        return 0;

err:
        fclose(fp);
        return -1;
}

static int
generic_flash_erase(struct REFLASH_TCP_T *h)
{
        char inbuf[1024] = { 0 };
        tcp_io(h, "FLASH ERASE\r", inbuf, sizeof(inbuf));
        return 0;
}

static int
t680_flash_erase(struct REFLASH_TCP_T *h)
{
        char inbuf[1024] = { 0 };
        tcp_io_sendonly(h, "FLASH ERASE\r");
        for (;;) {
                tcp_io_recvonly(h, inbuf, sizeof(inbuf));
                if (strstr(inbuf, "OK"))
                        break;
                else if (strstr(inbuf, "31"))
                        continue;
                else {
                        fprintf(stderr,
                                "Unexpected result of FLASH ERASE: '%s'\n",
                                inbuf);
                        exit(1);
                }
        }
        return 0;
}

static int
generic_flash_unlock(struct REFLASH_TCP_T *h)
{
        char inbuf[1024];
        tcp_io(h, "FLASH UNLOCK\r", inbuf, sizeof(inbuf));
        check_ok(inbuf);
        return 0;
}

static int
generic_reflash_(struct REFLASH_TCP_T *h, FILE *fp,
                 int (*ers)(struct REFLASH_TCP_T *))
{
        printf("Unlocking...\n");
        generic_flash_unlock(h);
        printf("Erasing...\n");
        ers(h);
        printf("Reflashing...\n");
        generic_flash_write(h, fp);
        return 0;
}

int
generic_reflash(struct REFLASH_TCP_T *h, FILE *fp)
{
        return generic_reflash_(h, fp, generic_flash_erase);
}

int
t680_reflash(struct REFLASH_TCP_T *h, FILE *fp)
{
        return generic_reflash_(h, fp, t680_flash_erase);
}
