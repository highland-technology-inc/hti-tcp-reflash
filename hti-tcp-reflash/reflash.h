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
#ifndef P620_REFLASH_H
#define  P620_REFLASH_H

#include <stdio.h>

struct  REFLASH_TCP_T;

extern int generic_reflash(struct REFLASH_TCP_T *h, FILE *fp);
extern int t680_reflash(struct REFLASH_TCP_T *h, FILE *fp);

extern struct REFLASH_TCP_T *tcp_open(const char *hostname);
extern int tcp_io(struct REFLASH_TCP_T *h, const char *out, char *in, int maxbytes);
extern int tcp_io_sendonly(struct REFLASH_TCP_T *h, const char *out);
extern int tcp_io_recvonly(struct REFLASH_TCP_T *n, char *in, int maxbytes);
extern int tcp_close(struct REFLASH_TCP_T *h);
extern int tcp_write(struct REFLASH_TCP_T *h, const char *out);
extern int tcp_read(struct REFLASH_TCP_T *h, char *in, int maxbytes);


#endif /* P620_REFLASH_H */
