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
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


enum {
        HTI_PORT = 2000,
};

static const char *HTI_SERVICE = "2000";

struct reflash_tcp_t {
        FILE *fp;
        char *lineptr;
        size_t n;
};

static int
open_remote_socket(const char *node, int socktype)
{
        struct addrinfo hints;
        struct addrinfo *list, *rp;
        int res, fd = -1;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = socktype;

        res = getaddrinfo(node, HTI_SERVICE, &hints, &list);
        if (res != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
                return -1;
        }

        for (rp = list; rp != NULL; rp = rp->ai_next) {
                struct sockaddr_in sin;

                if (rp->ai_family != AF_INET
                    || rp->ai_addrlen != sizeof(sin)) {
                        continue;
                }

                fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                if (fd == -1)
                        continue;

                memcpy(&sin, rp->ai_addr, sizeof(sin));
                sin.sin_port = HTI_PORT;
                printf("Trying %s...\n", inet_ntoa(sin.sin_addr));
                if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) {
                        char host[NI_MAXHOST];
                        const char *socktype_name = (socktype == SOCK_STREAM)
                                                   ? "TCP" : "UDP";
                        res = getnameinfo(rp->ai_addr, rp->ai_addrlen,
                                          host, sizeof(host), NULL, 0, 0);
                        if (res != 0) {
                                printf("Connecting via %s to <%s/%d>\n",
                                       socktype_name,
                                       inet_ntoa(sin.sin_addr),
                                       (unsigned int)sin.sin_port);
                        } else {
                                printf("Connecting via %s to <%s/%d> '%s'\n",
                                       socktype_name,
                                       inet_ntoa(sin.sin_addr),
                                       (unsigned int)sin.sin_port,
                                       host);
                        }
                        break;
                }
                close(fd);
                fd = -1;
        }

        freeaddrinfo(list);
        return fd;
}

static int
tcp_vfprintf(struct reflash_tcp_t *tcp, const char *fmt, va_list ap)
{
        int res = vfprintf(tcp->fp, fmt, ap);
        if (res < 0)
                return res;
        fputc('\r', tcp->fp);
        fflush(tcp->fp);
        return 0;
}

struct reflash_tcp_t *
tcp_open(const char *node)
{
        struct reflash_tcp_t *tcp = malloc(sizeof(*tcp));
        int fd;
        if (!tcp)
                goto e_tcp;
        memset(tcp, 0, sizeof(*tcp));
        fd = open_remote_socket(node, SOCK_STREAM);
        if (fd < 0)
                goto e_fd;
        tcp->fp = fdopen(fd, "r+");
        if (!tcp->fp)
                goto e_fp;

        return tcp;

e_fp:
        close(fd);
        fd = -1;
e_fd:
        free(tcp);
e_tcp:
        return NULL;
}

const char *
tcp_io(struct reflash_tcp_t *tcp, const char *fmt, ...)
{
        va_list ap;
        int res;

        va_start(ap, fmt);
        res = tcp_vfprintf(tcp, fmt, ap);
        va_end(ap);
        if (res < 0)
                return NULL;
        return tcp_getline(tcp);
}

int
tcp_io_sendonly(struct reflash_tcp_t *tcp, const char *fmt, ...)
{
        va_list ap;
        int res;

        va_start(ap, fmt);
        res = tcp_vfprintf(tcp, fmt, ap);
        va_end(ap);
        return res;
}

const char *
tcp_getline(struct reflash_tcp_t *tcp)
{
        if (sock_getline(&tcp->lineptr, &tcp->n, tcp->fp) < 0)
                return NULL;
        return tcp->lineptr;
}

void
tcp_close(struct reflash_tcp_t *tcp)
{
        if (tcp->lineptr != NULL)
                free(tcp->lineptr);
        if (tcp->fp != NULL)
                fclose(tcp->fp);
}

