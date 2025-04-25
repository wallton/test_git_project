/* dns_query.c: Standalone DNS Query Program
 *
 * Copyright (C) 2025 [Your Name or Organization]
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define DEBUG
#ifdef DEBUG
#define DPRINTF(X, args...) fprintf(stderr, X, ##args)
#else
#define DPRINTF(X, args...)
#endif

/* 常量定義 */
#define HFIXEDSZ 12
#define QFIXEDSZ 4
#define RRFIXEDSZ 10
#define PACKETSZ 512
#define MAXDNAME 1025
#define MAXCDNAME 255
#define NS_CMPRSFLGS 0xc0
#define C_IN 1
#define T_A 1
#define NOERROR 0
#define SERVFAIL 2
#define NO_RECOVERY 4
#define TRY_AGAIN 2
#define NO_DATA 4
#define RES_TIMEOUT 5
#define RES_DFLRETRY 2
#define NAMESERVER_PORT 53

/* 結構定義 */
struct resolv_header {
    int id;
    int qr, opcode, aa, tc, rd, ra, rcode;
    int qdcount;
    int ancount;
    int nscount;
    int arcount;
};

struct resolv_answer {
    char *dotted;
    int atype;
    int aclass;
    int ttl;
    int rdlength;
    const unsigned char *rdata;
    int rdoffset;
    char *buf;
    size_t buflen;
    size_t add_count;
};

typedef union sockaddr46_t {
    struct sockaddr sa;
    struct sockaddr_in sa4;
#ifdef __UCLIBC_HAS_IPV6__
    struct sockaddr_in6 sa6;
#endif
} sockaddr46_t;

/* 全局變數（模擬 resolv.c 的行為） */
static unsigned __nameservers = 0;
static sockaddr46_t *__nameserver = NULL;
static uint8_t __resolv_timeout = RES_TIMEOUT;
static uint8_t __resolv_attempts = RES_DFLRETRY;
static int h_errno = 0;

/* 輔助函數 */
static char *skip_nospace(char *p) {
    while (*p != '\0' && !isspace(*p)) {
        if (*p == '\n') {
            *p = '\0';
            break;
        }
        p++;
    }
    return p;
}

static char *skip_and_NUL_space(char *p) {
    while (1) {
        char c = *p;
        if (c == '\0' || !isspace(c))
            break;
        *p = '\0';
        if (c == '\n' || c == '#')
            break;
        p++;
    }
    return p;
}

void __open_nameservers(void) {
    char szBuffer[128];
    FILE *fp;
    sockaddr46_t sa;
    void *ptr;

    __nameservers = 0;
    if (__nameserver) {
        free(__nameserver);
        __nameserver = NULL;
    }

    fp = fopen("/etc/resolv.conf", "r");
    if (!fp) {
        DPRINTF("Failed to open /etc/resolv.conf\n");
        /* 預設使用本地 nameserver */
        __nameserver = malloc(sizeof(sockaddr46_t));
        if (__nameserver) {
            __nameserver[0].sa4.sin_family = AF_INET;
            __nameserver[0].sa4.sin_port = htons(NAMESERVER_PORT);
            __nameserver[0].sa4.sin_addr.s_addr = inet_addr("127.0.0.1");
            __nameservers = 1;
        }
        return;
    }

    while (fgets(szBuffer, sizeof(szBuffer), fp) != NULL) {
        char *keyword, *p;

        keyword = p = skip_and_NUL_space(szBuffer);
        p = skip_nospace(p);
        p = skip_and_NUL_space(p);

        if (strcmp(keyword, "nameserver") == 0) {
            *skip_nospace(p) = '\0';
            memset(&sa, 0, sizeof(sa));
            if (inet_pton(AF_INET, p, &sa.sa4.sin_addr) > 0) {
                sa.sa4.sin_family = AF_INET;
                sa.sa4.sin_port = htons(NAMESERVER_PORT);
            }
#ifdef __UCLIBC_HAS_IPV6__
            else if (inet_pton(AF_INET6, p, &sa.sa6.sin6_addr) > 0) {
                sa.sa6.sin6_family = AF_INET6;
                sa.sa6.sin6_port = htons(NAMESERVER_PORT);
            }
#endif
            else
                continue;

            ptr = realloc(__nameserver, (__nameservers + 1) * sizeof(sockaddr46_t));
            if (!ptr)
                continue;
            __nameserver = ptr;
            __nameserver[__nameservers++] = sa;
        }
    }
    fclose(fp);

    if (__nameservers == 0) {
        __nameserver = malloc(sizeof(sockaddr46_t));
        if (__nameserver) {
            __nameserver[0].sa4.sin_family = AF_INET;
            __nameserver[0].sa4.sin_port = htons(NAMESERVER_PORT);
            __nameserver[0].sa4.sin_addr.s_addr = inet_addr("127.0.0.1");
            __nameservers = 1;
        }
    }
}

void __close_nameservers(void) {
    if (__nameserver) {
        free(__nameserver);
        __nameserver = NULL;
    }
    __nameservers = 0;
}

int __encode_header(struct resolv_header *h, unsigned char *dest, int maxlen) {
    if (maxlen < HFIXEDSZ)
        return -1;

    dest[0] = (h->id & 0xff00) >> 8;
    dest[1] = (h->id & 0x00ff) >> 0;
    dest[2] = (h->qr ? 0x80 : 0) |
              ((h->opcode & 0x0f) << 3) |
              (h->aa ? 0x04 : 0) |
              (h->tc ? 0x02 : 0) |
              (h->rd ? 0x01 : 0);
    dest[3] = (h->ra ? 0x80 : 0) | (h->rcode & 0x0f);
    dest[4] = (h->qdcount & 0xff00) >> 8;
    dest[5] = (h->qdcount & 0x00ff) >> 0;
    dest[6] = (h->ancount & 0xff00) >> 8;
    dest[7] = (h->ancount & 0x00ff) >> 0;
    dest[8] = (h->nscount & 0xff00) >> 8;
    dest[9] = (h->nscount & 0x00ff) >> 0;
    dest[10] = (h->arcount & 0xff00) >> 8;
    dest[11] = (h->arcount & 0x00ff) >> 0;

    return HFIXEDSZ;
}

void __decode_header(unsigned char *data, struct resolv_header *h) {
    h->id = (data[0] << 8) | data[1];
    h->qr = (data[2] & 0x80) ? 1 : 0;
    h->opcode = (data[2] >> 3) & 0x0f;
    h->aa = (data[2] & 0x04) ? 1 : 0;
    h->tc = (data[2] & 0x02) ? 1 : 0;
    h->rd = (data[2] & 0x01) ? 1 : 0;
    h->ra = (data[3] & 0x80) ? 1 : 0;
    h->rcode = data[3] & 0x0f;
    h->qdcount = (data[4] << 8) | data[5];
    h->ancount = (data[6] << 8) | data[7];
    h->nscount = (data[8] << 8) | data[9];
    h->arcount = (data[10] << 8) | data[11];
}

int __encode_dotted(const char *dotted, unsigned char *dest, int maxlen) {
    int len = 0, i;
    const char *p = dotted;
    unsigned char *q = dest;

    while (*p) {
        char label[63];
        int label_len = 0;

        while (*p && *p != '.' && label_len < 63) {
            label[label_len++] = *p++;
        }

        if (label_len == 0)
            break;

        if (len + label_len + 1 > maxlen)
            return -1;

        *q++ = label_len;
        memcpy(q, label, label_len);
        q += label_len;
        len += label_len + 1;

        if (*p == '.')
            p++;
    }

    if (len + 1 > maxlen)
        return -1;

    *q++ = 0;
    len++;

    return len;
}

int __encode_question(const struct resolv_question *q, unsigned char *dest, int maxlen) {
    int i;

    i = __encode_dotted(q->dotted, dest, maxlen);
    if (i < 0)
        return i;

    dest += i;
    maxlen -= i;

    if (maxlen < 4)
        return -1;

    dest[0] = (q->qtype & 0xff00) >> 8;
    dest[1] = (q->qtype & 0x00ff) >> 0;
    dest[2] = (q->qclass & 0xff00) >> 8;
    dest[3] = (q->qclass & 0x00ff) >> 0;

    return i + 4;
}

int __form_query(int id, const char *name, int type, unsigned char *packet, int maxlen) {
    struct resolv_header h;
    struct resolv_question q;
    int i, j;

    memset(&h, 0, sizeof(h));
    h.id = id;
    h.qdcount = 1;

    q.dotted = (char *)name;
    q.qtype = type;
    q.qclass = C_IN;

    i = __encode_header(&h, packet, maxlen);
    if (i < 0)
        return i;

    j = __encode_question(&q, packet + i, maxlen - i);
    if (j < 0)
        return j;

    return i + j;
}

int __decode_dotted(const unsigned char *packet, int offset, int packet_len, char *dest, int dest_len) {
    int len = 0, i;
    const unsigned char *p = packet + offset;
    char *q = dest;

    while (*p && len < dest_len - 1) {
        if ((*p & NS_CMPRSFLGS) == NS_CMPRSFLGS) {
            /* 壓縮名稱，尚未實現 */
            return -1;
        }

        i = *p++;
        if (i > 63 || len + i + 1 >= dest_len)
            return -1;

        memcpy(q, p, i);
        q += i;
        p += i;
        len += i;

        if (*p && len < dest_len - 1) {
            *q++ = '.';
            len++;
        }
    }

    *q = '\0';
    return p - (packet + offset);
}

int __decode_answer(const unsigned char *message, int offset, int len, struct resolv_answer *a) {
    char temp[MAXDNAME];
    int i;

    i = __decode_dotted(message, offset, len, temp, sizeof(temp));
    if (i < 0)
        return i;

    message += offset + i;
    len -= i + RRFIXEDSZ + offset;
    if (len < 0)
        return len;

    a->dotted = strdup(temp);
    if (!a->dotted)
        return -1;

    a->atype = (message[0] << 8) | message[1];
    message += 2;
    a->aclass = (message[0] << 8) | message[1];
    message += 2;
    a->ttl = (message[0] << 24) | (message[1] << 16) | (message[2] << 8) | message[3];
    message += 4;
    a->rdlength = (message[0] << 8) | message[1];
    message += 2;
    a->rdata = message;
    a->rdoffset = offset + i + RRFIXEDSZ;

    if (len < a->rdlength)
        return -1;

    return i + RRFIXEDSZ + a->rdlength;
}

int __dns_lookup(const char *name, int type, unsigned char **outpacket, struct resolv_answer *a) {
    int fd, i, len, ret;
    unsigned char *packet = malloc(PACKETSZ);
    struct resolv_header h;
    unsigned nameserver_idx = 0;
    int retry_count = __resolv_attempts;
    struct pollfd pfd;

    if (!packet)
        return -1;

    memset(a, 0, sizeof(*a));

try_next_server:
    if (nameserver_idx >= __nameservers || retry_count <= 0) {
        free(packet);
        return -1;
    }

    fd = socket(__nameserver[nameserver_idx].sa.sa_family, SOCK_DGRAM, 0);
    if (fd < 0) {
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    memset(&h, 0, sizeof(h));
    h.id = getpid() & 0xffff;
    h.qdcount = 1;
    h.rd = 1;

    len = __form_query(h.id, name, type, packet, PACKETSZ);
    if (len < 0) {
        close(fd);
        free(packet);
        return -1;
    }

    ret = sendto(fd, packet, len, 0, &__nameserver[nameserver_idx].sa, sizeof(__nameserver[nameserver_idx]));
    if (ret != len) {
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    pfd.fd = fd;
    pfd.events = POLLIN;
    ret = poll(&pfd, 1, __resolv_timeout * 1000);
    if (ret <= 0) {
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    ret = recv(fd, packet, PACKETSZ, 0);
    if (ret < HFIXEDSZ) {
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    __decode_header(packet, &h);
    if (h.id != (getpid() & 0xffff) || h.rcode != NOERROR) {
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    close(fd);
    *outpacket = packet;
    return ret;
}

int res_query(const char *dname, int class, int type, unsigned char *answer, int anslen) {
    int i;
    unsigned char *packet = NULL;
    struct resolv_answer a;
    struct resolv_header h;

    if (!dname || class != C_IN) {
        h_errno = NO_RECOVERY;
        return -1;
    }

    __open_nameservers();
    if (__nameservers == 0) {
        h_errno = NO_ADDRESS;
        return -1;
    }

    memset(&a, 0, sizeof(a));
    i = __dns_lookup(dname, type, &packet, &a);
    if (i < 0) {
        if (!h_errno)
            h_errno = TRY_AGAIN;
        return -1;
    }

    __decode_header(packet, &h);
    if (h.tc) {
        /* TC bit 設置，切換到 TCP */
        free(packet);
        free(a.dotted);
        return res_query_tcp(dname, class, type, answer, anslen);
    }

    free(a.dotted);
    if (i > anslen)
        i = anslen;
    memcpy(answer, packet, i);
    free(packet);
    return i;
}

int res_query_tcp(const char *dname, int class, int type, unsigned char *answer, int anslen) {
    struct resolv_header h;
    unsigned char *packet = NULL;
    int fd = -1, i, len, ret;
    unsigned char *buf = NULL;
    int bufsize = 65536;
    unsigned nameserver_idx;
    struct pollfd pfd;
    int poll_timeout;
    int retry_count;

    if (!dname || class != C_IN || !answer || anslen < HFIXEDSZ) {
        h_errno = NO_RECOVERY;
        return -1;
    }

    packet = malloc(PACKETSZ);
    if (!packet) {
        h_errno = NETDB_INTERNAL;
        errno = ENOMEM;
        return -1;
    }

    buf = malloc(bufsize);
    if (!buf) {
        free(packet);
        h_errno = NETDB_INTERNAL;
        errno = ENOMEM;
        return -1;
    }

    __open_nameservers();
    if (__nameservers == 0) {
        free(packet);
        free(buf);
        h_errno = NO_ADDRESS;
        return -1;
    }

    memset(&h, 0, sizeof(h));
    h.id = getpid() & 0xffff;
    h.qdcount = 1;
    h.rd = 1;
    len = __form_query(h.id, dname, type, packet, PACKETSZ);
    if (len < 0) {
        free(packet);
        free(buf);
        h_errno = NO_RECOVERY;
        return -1;
    }

    retry_count = __resolv_attempts;
    nameserver_idx = 0;

try_next_server:
    if (nameserver_idx >= __nameservers || retry_count <= 0) {
        free(packet);
        free(buf);
        h_errno = TRY_AGAIN;
        return -1;
    }

    fd = socket(__nameserver[nameserver_idx].sa.sa_family, SOCK_STREAM, 0);
    if (fd < 0) {
        DPRINTF("socket creation failed: %s\n", strerror(errno));
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    if (connect(fd, &__nameserver[nameserver_idx].sa, sizeof(__nameserver[nameserver_idx])) < 0) {
        DPRINTF("connect failed: %s\n", strerror(errno));
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    buf[0] = (len >> 8) & 0xff;
    buf[1] = len & 0xff;
    memcpy(buf + 2, packet, len);
    ret = send(fd, buf, len + 2, 0);
    if (ret != len + 2) {
        DPRINTF("send failed: %s\n", strerror(errno));
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    pfd.fd = fd;
    pfd.events = POLLIN;
    poll_timeout = __resolv_timeout * 1000;

    ret = poll(&pfd, 1, poll_timeout);
    if (ret <= 0) {
        DPRINTF("poll timeout or error: %s\n", strerror(errno));
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    if (!(pfd.revents & POLLIN)) {
        DPRINTF("poll returned no input: revents=%d\n", pfd.revents);
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    ret = recv(fd, buf, 2, MSG_WAITALL);
    if (ret != 2) {
        DPRINTF("failed to read length prefix: %s\n", strerror(errno));
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    len = (buf[0] << 8) | buf[1];
    if (len > anslen || len > bufsize - 2) {
        DPRINTF("response too large: %d\n", len);
        close(fd);
        free(packet);
        free(buf);
        h_errno = NO_RECOVERY;
        errno = EMSGSIZE;
        return -1;
    }

    ret = recv(fd, buf, len, MSG_WAITALL);
    if (ret != len) {
        DPRINTF("incomplete response: expected %d, got %d\n", len, ret);
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    __decode_header(buf, &h);
    if (h.id != (getpid() & 0xffff)) {
        DPRINTF("ID mismatch: expected %d, got %d\n", getpid() & 0xffff, h.id);
        close(fd);
        nameserver_idx++;
        retry_count--;
        goto try_next_server;
    }

    if (h.rcode != NOERROR) {
        DPRINTF("DNS error: rcode=%d\n", h.rcode);
        close(fd);
        free(packet);
        free(buf);
        h_errno = h.rcode == SERVFAIL ? TRY_AGAIN : NO_DATA;
        return -1;
    }

    if (len > anslen)
        len = anslen;
    memcpy(answer, buf, len);

    close(fd);
    free(packet);
    free(buf);
    return len;
}

/* 解析回應並提取 IP 地址 */
void parse_answer(unsigned char *answer, int anslen) {
    struct resolv_header h;
    struct resolv_answer a;
    int offset = HFIXEDSZ;
    int i;

    __decode_header(answer, &h);
    if (h.qdcount > 0) {
        /* 跳過問題部分 */
        offset += __decode_dotted(answer, offset, anslen, a.dotted, MAXDNAME);
        offset += 4; /* QTYPE 和 QCLASS */
    }

    for (i = 0; i < h.ancount; i++) {
        memset(&a, 0, sizeof(a));
        int ret = __decode_answer(answer, offset, anslen, &a);
        if (ret < 0) {
            DPRINTF("Failed to decode answer\n");
            break;
        }

        if (a.atype == T_A && a.aclass == C_IN && a.rdlength == 4) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, a.rdata, ip, sizeof(ip));
            printf("IP: %s\n", ip);
        }
        free(a.dotted);
        offset += ret;
    }
}

int main(int argc, char *argv[]) {
    unsigned char answer[65536];
    int len;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <domain>\n", argv[0]);
        return 1;
    }

    len = res_query(argv[1], C_IN, T_A, answer, sizeof(answer));
    if (len < 0) {
        fprintf(stderr, "Query failed: h_errno=%d\n", h_errno);
        __close_nameservers();
        return 1;
    }

    printf("Query succeeded, received %d bytes\n", len);
    parse_answer(answer, len);

    __close_nameservers();
    return 0;
}