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
