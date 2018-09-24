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
 *
 * REVISIT: The entire implementation of this API looks like overkill.  Why
 * create two sockets?
 */
#include "reflash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <assert.h>

#define HOSTNAME_SIZE       32
#define RXBUF_SIZE          1024
#define TXBUF_SIZE          256

struct REFLASH_TCP_T {
        char                hostname[HOSTNAME_SIZE];
        int                 ep_sock;
        int                 server_sock;
        struct sockaddr_in  ep;
        struct sockaddr_in  server;
        int                 opt_val;
};

static char dummy_rxbuf[256];

static void
tcp_send(int socketfd, const char *command)
{
        int ret = send(socketfd, command, strlen(command), 0);
        if (ret < 0) {
                fprintf(stderr, "TCP send(\"%s\") failed\n", command);
                exit(1);
        }
}

static int
tcp_recv(int socketfd, char *buf, int maxbytes)
{
        int ret = recv(socketfd, buf, maxbytes, 0);
        if (ret < 0) {
                fprintf(stderr, "TCP no reply.\n");
                exit(1);
        } else if (ret == 0) {
                fprintf(stderr, "TCP unexpected EOF.\n");
                exit(1);
        } else if (buf[0] == '\0') {
                fprintf(stderr, "TCP recv: Unexpected string of length zero.\n");
                exit(1);
        } else if (buf[0] == 'E' && buf[3] == ':') {
                fprintf(stderr, "target error: \"%s\"\n", buf);
                exit(1);
        }
        return ret;
}

static int
make_socket(uint16_t port, const char *hostname, struct sockaddr_in *serv_addr)
{
        int sockfd;
        struct hostent *hp;

        /* Create the socket.  */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
                fputs("ERROR opening socket\n", stderr);
                exit (1);
        }

        /* Initialize socket structure */
        memset((char *)serv_addr, 0, sizeof(*serv_addr));
        hp = gethostbyname(hostname);
        if (hp == NULL) {
                fprintf(stderr, "Host name %s not on network\n", hostname);
                exit(1);
        }
        serv_addr->sin_family = hp->h_addrtype;
        /*
         * TODO: Should I not also verify sin_family before deciding
         * copy size?
         */
        memcpy(&serv_addr->sin_addr.s_addr, *hp->h_addr_list,
               sizeof(serv_addr->sin_addr.s_addr));
        serv_addr->sin_port = htons(port);

        return sockfd;
}

static int
make_host_socket(struct REFLASH_TCP_T *h)
{
        int sockfd, status;

        sockfd = make_socket(7000, "localhost", &h->server);

        h->opt_val  = 1;
        status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                             &h->opt_val, (socklen_t)(sizeof(h->opt_val)));
        if (status != 0)
                fprintf(stderr, "Warning, could not set socket for reusable address\n");

        /* Bind the host address */
        status = bind(sockfd, (struct sockaddr *)&h->server, sizeof(h->server));
        if (status < 0) {
                fprintf(stderr, "ERROR on binding: %s\n", strerror(errno));
                exit(1);
        }

        /* Connect to the socket */
        status = connect(sockfd, (struct sockaddr *)&h->server, sizeof(h->server));
        if (status < 0) {
                fprintf(stderr, "ERROR on connecting: %s\n", strerror(errno));
                exit(1);
        }
        return sockfd;
}

static int
make_ep_socket(struct REFLASH_TCP_T *h, const char *hostname)
{
        int sockfd, status;

        sockfd = make_socket(2000, hostname, &h->server);

        /* Connect to the socket */
        status = connect(sockfd, (struct sockaddr *)&h->server, sizeof(h->server));
        if (status < 0) {
                fprintf(stderr, "ERROR on connecting: %s\n", strerror(errno));
                exit(1);
        }
        return sockfd;
}


/* *********************************************************************
 *                      Public functions
 **********************************************************************/

struct REFLASH_TCP_T *
tcp_open(const char *hostname)
{
        struct REFLASH_TCP_T *h = calloc(1, sizeof(*h));
        if (h == NULL)
                goto errmalloc;

        strncpy(h->hostname, hostname, HOSTNAME_SIZE);
        h->server_sock = make_host_socket(h);
        h->ep_sock = make_ep_socket(h, h->hostname);

        /* Flush characters by sending NL and ignoring 1st error */
        tcp_send(h->ep_sock, "\n");
        tcp_recv(h->ep_sock, dummy_rxbuf, sizeof(dummy_rxbuf));

        return h;

        free(h);
errmalloc:
        return NULL;
}

int
tcp_write(struct REFLASH_TCP_T *h, const char *out)
{
        tcp_send(h->ep_sock, out);
        return 0;
}

int
tcp_read(struct REFLASH_TCP_T *h, char *in, int maxbytes)
{
        return tcp_recv(h->ep_sock, in, maxbytes - 1);
}

int
tcp_io(struct REFLASH_TCP_T *h, const char *out, char *in, int maxbytes)
{
        tcp_send(h->ep_sock, out);
        tcp_recv(h->ep_sock, in, maxbytes - 1);
        return 0;
}

int
tcp_io_sendonly(struct REFLASH_TCP_T *h, const char *out)
{
        tcp_send(h->ep_sock, out);
        return 0;
}

int
tcp_io_recvonly(struct REFLASH_TCP_T *h, char *in, int maxbytes)
{
        tcp_recv(h->ep_sock, in, maxbytes - 1);
        in[maxbytes - 1] = '\0';
        return 0;
}

int
tcp_close(struct REFLASH_TCP_T *h)
{
        if (h == NULL)
                return -EINVAL;

        close(h->ep_sock);
        close(h->server_sock);
        free(h);
        return 0;
}
