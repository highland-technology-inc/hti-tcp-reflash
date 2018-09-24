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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct target_lut_t {
        const char *name;
        int (*reflash)(struct REFLASH_TCP_T *tcp, FILE *fp);
} target_lut[] = {
        { "p620", generic_reflash },
        { "p470", generic_reflash },
        { "p330", generic_reflash },
        { "t680", t680_reflash },
        { "v120", t680_reflash },
        { "v124", t680_reflash },
        { NULL, NULL },
};

static const struct target_lut_t *
target_lookup(const char *target)
{
        int i;
        for (i = 0; target_lut[i].name != NULL; ++i) {
                if (!strcmp(target, target_lut[i].name))
                        return &target_lut[i];
        }
        return NULL;
}

int
main(int argc, char **argv)
{
        struct REFLASH_TCP_T *h;
        /* default caltable's serial number */
        int serial = -1;
        int opt;
        FILE *fp;
        char hostname[16];
        char *ip = NULL;
        const struct target_lut_t *lut;

        /* TODO: Add '-i' for direct IP address */
        while ((opt = getopt(argc, argv, "s:")) != -1) {
                switch (opt) {
                case 's':
                        serial = atoi(optarg);
                        break;
                default:
                        fprintf(stderr, "Usage: %s [-s serial] target filename\n",
                                argv[0]);
                        exit(1);
                        break;
                }
        }

        if (!((serial < 0) ^ (ip == NULL))) {
                fprintf(stderr, "Expected: one of -s or -i\n");
                exit(1);
        }

        if (argc - optind < 2) {
                fprintf(stderr, "Expected: TARGET FILENAME\n");
                exit(1);
        }

        if ((lut = target_lookup(argv[optind])) == NULL) {
                fprintf(stderr, "Invalid target '%s'\n",
                        argv[optind]);
                exit(1);
        }

        fp = fopen(argv[optind + 1], "r");
        if (fp == NULL) {
                perror("Cannot open reflash file");
                exit(1);
        }

        snprintf(hostname, sizeof(hostname), "%s-%05u", lut->name, serial);
        hostname[sizeof(hostname) - 1] = '\0';
        h = tcp_open(hostname);
        if (h == NULL) {
                perror("TCP open failed");
                exit(1);
        }

        printf("Wait\n");
        if (lut->reflash(h, fp) < 0)
                fprintf(stderr, "%s: reflash failed\n", argv[0]);
        else
                printf("Done\n");

        tcp_close(h);
        return 0;
}
