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
#include <resolv.h>
#include <ctype.h> /* isspace */

#define DEBUG
#ifdef DEBUG
#define DPRINTF(X, args...) fprintf(stderr, X, ##args)
#else
#define DPRINTF(X, args...)
#endif

/* 常量定義 */
/*
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
*/
#define MAX_ALIASES 4
#define MAX_ADDRS 16

/* 結構定義 */
struct resolv_header {
    int id;
    int qr, opcode, aa, tc, rd, ra, rcode;
    int qdcount;
    int ancount;
    int nscount;
    int arcount;
};

struct resolv_question {
	char *dotted;
	int qtype;
	int qclass;
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
    struct sockaddr_in6 sa6;
} sockaddr46_t;

/* 全局變數（模擬 resolv.c 的行為） */
static unsigned __nameservers = 0;
static sockaddr46_t *__nameserver = NULL;
static uint8_t __resolv_timeout = RES_TIMEOUT;
static uint8_t __resolv_attempts = RES_DFLRETRY;
static char *user_dns_setting = NULL;

/* gethostbyname2 相關結構 */
static struct hostent __hostent;
static char __hostent_buf[1024];
static char * __hostent_aliases[MAX_ALIASES];
static char * __hostent_addr_list[MAX_ADDRS];
static char __hostent_addr_buf[MAX_ADDRS * (sizeof(struct in6_addr) > sizeof(struct in_addr) ? sizeof(struct in6_addr) : sizeof(struct in_addr))];


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

char * retIP_address(sockaddr46_t *srv)
{
    static char ipAddr_str[64] = {0};
    if(srv)
    {
        if( AF_INET == srv->sa.sa_family )
        {
            inet_ntop(AF_INET, &(srv->sa4.sin_addr), ipAddr_str, 64);
        }
        else if ( AF_INET6 == srv->sa.sa_family )
        {
            inet_ntop(AF_INET6, &(srv->sa6.sin6_addr), ipAddr_str, 64);
        }
    }
    return &ipAddr_str[0];
}

void __open_nameservers_with_user_dns(const char *user_dns) {
    sockaddr46_t sa;

    /* 清空現有的 user_dns_setting */
    if (user_dns_setting) {
        free(user_dns_setting);
        user_dns_setting = NULL;
    }

    /* 如果使用者指定了 DNS 伺服器，驗證並設置 */
    if (user_dns && *user_dns)
    {
        memset(&sa, 0, sizeof(sa));
        if (inet_pton(AF_INET, user_dns, &sa.sa4.sin_addr) > 0) {
            sa.sa4.sin_family = AF_INET;
            sa.sa4.sin_port = htons(NAMESERVER_PORT);
        }
        else if (inet_pton(AF_INET6, user_dns, &sa.sa6.sin6_addr) > 0) {
            sa.sa6.sin6_family = AF_INET6;
            sa.sa6.sin6_port = htons(NAMESERVER_PORT);
        }
        else {
            DPRINTF("Invalid DNS server address: %s\n", user_dns);
            return;
        }

        /* 儲存 user_dns 並初始化 __nameserver */
        user_dns_setting = strdup(user_dns);
        if (!user_dns_setting) {
            DPRINTF("Failed to allocate memory for user_dns_setting\n");
            return;
        }
    }
}

void __open_nameservers(void) {
    char szBuffer[128];
    FILE *fp = NULL;
    sockaddr46_t sa;
    void *ptr;

    __nameservers = 0;
    if (__nameserver) {
        free(__nameserver);
        __nameserver = NULL;
    }

    /* 如果使用者指定了 DNS server，優先使用 */
    if (user_dns_setting && *user_dns_setting)
    {
        memset(&sa, 0, sizeof(sa));
        if (inet_pton(AF_INET, user_dns_setting, &sa.sa4.sin_addr) > 0) {
            sa.sa4.sin_family = AF_INET;
            sa.sa4.sin_port = htons(NAMESERVER_PORT);
        }
        else if (inet_pton(AF_INET6, user_dns_setting, &sa.sa6.sin6_addr) > 0) {
            sa.sa6.sin6_family = AF_INET6;
            sa.sa6.sin6_port = htons(NAMESERVER_PORT);
        }
        else {
            DPRINTF("Invalid user DNS server address: %s\n", user_dns_setting);
            /* 清空無效的 user_dns_setting，繼續後續邏輯 */
            free(user_dns_setting);
            user_dns_setting = NULL;
        }

        if (user_dns_setting) {
            __nameserver = malloc(sizeof(sockaddr46_t));
            if (__nameserver) {
                __nameserver[0] = sa;
                __nameservers = 1;
                return;
            }
            DPRINTF("Failed to allocate memory for user nameserver\n");
            free(user_dns_setting);
            user_dns_setting = NULL;
        }
    }


    /* 否則從 /etc/resolv.conf 讀取 */
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
            else if (inet_pton(AF_INET6, p, &sa.sa6.sin6_addr) > 0) {
                sa.sa6.sin6_family = AF_INET6;
                sa.sa6.sin6_port = htons(NAMESERVER_PORT);
            }
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

    if (user_dns_setting) {
        free(user_dns_setting);
        user_dns_setting = NULL;
    }
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
    h.rd = 1;

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
#if 0
int __decode_dotted(const unsigned char *packet, int offset, int packet_len, char *dest, int dest_len) {
    int len = 0, i;
    const unsigned char *p = packet + offset;
    char *q = dest;

printf("billy(%s# %d) NS_CMPRSFLGS (0x%x) \n", __func__, __LINE__, NS_CMPRSFLGS); 

    while (*p && len < dest_len - 1) {

printf("billy(%s# %d) *p= 0x%x \n", __func__, __LINE__, *p);

        if ((*p & NS_CMPRSFLGS) == NS_CMPRSFLGS) {
            /* 壓縮名稱，尚未實現 */
printf("billy(%s# %d) ----- \n", __func__, __LINE__);
            return -1;
        }

        i = *p++;
        if (i > 63 || len + i + 1 >= dest_len) {
printf("billy(%s# %d) i(%d) x=%d dest_len=%d\n", __func__, __LINE__, i, (len + i + 1), dest_len);
            return -1;
        }

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

#endif
/* Decode a dotted string from nameserver transport-level encoding.
   This routine understands compressed data. */
int __decode_dotted(const unsigned char *packet, int offset, int packet_len, char *dest, int dest_len)
{
	unsigned b;
	unsigned measure = 1;
	unsigned total = 0;
	unsigned used = 0;

	if (!packet)
		return -1;

	while (1) {
		if (offset >= packet_len)
			return -1;
        
		b = packet[offset++];

printf("billy(%s# %d) b= 0x%x \n", __func__, __LINE__, b);

		if (b == 0)
			break;

		if (measure)
			total++;

		if ((b & 0xc0) == 0xc0) {
			if (offset >= packet_len)
				return -1;
			if (measure)
				total++;
			/* compressed item, redirect */
			offset = ((b & 0x3f) << 8) | packet[offset];
			measure = 0;
			continue;
		}

		if (used + b + 1 >= dest_len)
			return -1;
		if (offset + b >= packet_len)
			return -1;
		memcpy(dest + used, packet + offset, b);
		offset += b;
		used += b;

		if (measure)
			total += b;

		if (packet[offset] != 0)
			dest[used++] = '.';
		else
			dest[used++] = '\0';
	}

	/* The null byte must be counted too */
	if (measure)
		total++;

	//DPRINTF("Total decode len = %d\n", total);

	return total;
}


int __decode_answer(const unsigned char *message, int offset, int len, struct resolv_answer *a) {
    char temp[MAXDNAME];
    int i;

    i = __decode_dotted(message, offset, len, temp, sizeof(temp));
printf("billy(%s# %d) i(%d)\n", __func__, __LINE__, i);
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

printf("\nTry DNS Server : %s\n\n", retIP_address(&__nameserver[nameserver_idx]) );

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

printf("\nTry DNS Server : %s\n\n", retIP_address(&__nameserver[nameserver_idx]) );

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

int res_query(const char *dname, int class, int type, unsigned char *answer, int anslen) {
    int i;
    unsigned char *packet = NULL;
    struct resolv_answer a;
    struct resolv_header h;

    if (!dname || class != C_IN) {
        h_errno = NO_RECOVERY;
        return -1;
    }

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

/* gethostbyname2 實現 */
struct hostent *gethostbyname2(const char *name, int af) {
    unsigned char answer[65536] = {0};
    char temp[MAXDNAME] = {0};
    int len, type, i, offset;
    struct resolv_header h;
    struct resolv_answer a;
    char *bufptr = __hostent_buf;
    char *addrptr = __hostent_addr_buf;
    int addr_count = 0;
    size_t bufsize = sizeof(__hostent_buf);
    size_t addr_bufsize = sizeof(__hostent_addr_buf);

    if (!name || (af != AF_INET && af != AF_INET6)) {
        h_errno = NO_RECOVERY;
        return NULL;
    }

    /* 如果尚未初始化 nameserver，調用 __open_nameservers */
    if (__nameservers == 0) {
        __open_nameservers();
        if (__nameservers == 0) {
            h_errno = NO_ADDRESS;
            return NULL;
        }
    }


    type = (af == AF_INET) ? T_A : T_AAAA;
    len = res_query(name, C_IN, type, answer, sizeof(answer));
    if (len < 0) {
        __close_nameservers();
        return NULL;
    }

    /* 初始化 hostent */
    memset(&__hostent, 0, sizeof(__hostent));
    memset(__hostent_aliases, 0, sizeof(__hostent_aliases));
    memset(__hostent_addr_list, 0, sizeof(__hostent_addr_list));
    memset(__hostent_buf, 0, sizeof(__hostent_buf));
    memset(__hostent_addr_buf, 0, sizeof(__hostent_addr_buf));

    /* 設置 hostent 的基本資訊 */
    __hostent.h_name = bufptr;
    strncpy(bufptr, name, bufsize);
    bufptr += strlen(name) + 1;
    bufsize -= strlen(name) + 1;
    __hostent.h_aliases = __hostent_aliases;
    __hostent.h_addrtype = af;
    __hostent.h_length = (af == AF_INET) ? sizeof(struct in_addr) : sizeof(struct in6_addr);
    __hostent.h_addr_list = __hostent_addr_list;

//printf("billy(%s# %d) HFIXEDSZ(%d)\n", __func__, __LINE__, HFIXEDSZ);

    /* 解析回應 */
    __decode_header(answer, &h);
    offset = HFIXEDSZ;
    if (h.qdcount > 0) {
        offset += __decode_dotted(answer, offset, len, temp, MAXDNAME);
//printf("billy(%s# %d) offset(%d) \n", __func__, __LINE__, offset);
        offset += 4; /* QTYPE 和 QCLASS */
    }

//printf("billy(%s# %d) offset(%d) len(%d)\n", __func__, __LINE__, offset, len);

    for (i = 0; i < h.ancount && addr_count < MAX_ADDRS; i++) {
        memset(&a, 0, sizeof(a));
        int ret = __decode_answer(answer, offset, len, &a);
printf("billy(%s# %d) ret(%d) offset(%d) \n", __func__, __LINE__, ret, offset);
        if (ret < 0) {
            DPRINTF("Failed to decode answer\n");
            free(a.dotted);
            break;
        }

        if (a.atype == type && a.aclass == C_IN && a.rdlength == __hostent.h_length) {
            if (addr_bufsize < a.rdlength) {
                DPRINTF("Address buffer overflow\n");
                free(a.dotted);
                break;
            }
            memcpy(addrptr, a.rdata, a.rdlength);
            __hostent_addr_list[addr_count] = addrptr;
            addrptr += a.rdlength;
            addr_bufsize -= a.rdlength;
            addr_count++;
        }
        free(a.dotted);
        offset += ret;
    }

    __hostent_addr_list[addr_count] = NULL;
    if (addr_count == 0) {
        h_errno = NO_DATA;
        __close_nameservers();
        return NULL;
    }

    __close_nameservers();
    return &__hostent;
}

/* 解析並輸出 hostent 結果 */
void print_hostent(struct hostent *h) {
    int i;
    char addr_str[INET6_ADDRSTRLEN];

    if (!h) {
        printf("No results found\n");
        return;
    }

    printf("Name: %s\n", h->h_name);
    for (i = 0; h->h_addr_list[i]; i++) {
        inet_ntop(h->h_addrtype, h->h_addr_list[i], addr_str, sizeof(addr_str));
        printf("Address (%s): %s\n", h->h_addrtype == AF_INET ? "IPv4" : "IPv6", addr_str);
    }
}


static int parse_reply(const unsigned char *msg, size_t len)
{
	HEADER *header;

	ns_msg handle;
	ns_rr rr;
	int i, n, rdlen;
	const char *format = NULL;
	char astr[INET6_ADDRSTRLEN], dname[MAXDNAME];
	const unsigned char *cp;

	header = (HEADER *)msg;
	if (!header->aa)
		printf("Non-authoritative answer:\n");

	if (ns_initparse(msg, len, &handle) != 0) {
		printf("Unable to parse reply: %s\n", strerror(errno));
		return -1;
	}

	for (i = 0; i < ns_msg_count(handle, ns_s_an); i++)
    {
		if (ns_parserr(&handle, ns_s_an, i, &rr) != 0) {
			printf("Unable to parse resource record: %s\n", strerror(errno));
			return -1;
		}

		rdlen = ns_rr_rdlen(rr);

		switch (ns_rr_type(rr))
		{
		case ns_t_a:
			if (rdlen != 4) {
				printf("unexpected A record length %d\n", rdlen);
				return -1;
			}
			inet_ntop(AF_INET, ns_rr_rdata(rr), astr, sizeof(astr));
			printf("Name:\t%s\nAddress: %s\n", ns_rr_name(rr), astr);
			break;


		case ns_t_aaaa:
			if (rdlen != 16) {
				printf("unexpected AAAA record length %d\n", rdlen);
				return -1;
			}
			inet_ntop(AF_INET6, ns_rr_rdata(rr), astr, sizeof(astr));
			/* bind-utils-9.11.3 uses the same format for A and AAAA answers */
			printf("Name:\t%s\nAddress: %s\n", ns_rr_name(rr), astr);
			break;


		case ns_t_ns:
			if (!format)
				format = "%s\tnameserver = %s\n";
			/* fall through */

		case ns_t_cname:
			if (!format)
				format = "%s\tcanonical name = %s\n";
			/* fall through */

		case ns_t_ptr:
			if (!format)
				format = "%s\tname = %s\n";
			if (ns_name_uncompress(ns_msg_base(handle), ns_msg_end(handle),
					ns_rr_rdata(rr), dname, sizeof(dname)) < 0
			) {
				//printf("Unable to uncompress domain: %s\n", strerror(errno));
				return -1;
			}
			printf(format, ns_rr_name(rr), dname);
			break;

		case ns_t_mx:
			if (rdlen < 2) {
				printf("MX record too short\n");
				return -1;
			}
			n = ns_get16(ns_rr_rdata(rr));
			if (ns_name_uncompress(ns_msg_base(handle), ns_msg_end(handle),
					ns_rr_rdata(rr) + 2, dname, sizeof(dname)) < 0
			) {
				//printf("Cannot uncompress MX domain: %s\n", strerror(errno));
				return -1;
			}
			printf("%s\tmail exchanger = %d %s\n", ns_rr_name(rr), n, dname);
			break;

		case ns_t_txt:
			if (rdlen < 1) {
				//printf("TXT record too short\n");
				return -1;
			}
			n = *(unsigned char *)ns_rr_rdata(rr);
			if (n > 0) {
				memset(dname, 0, sizeof(dname));
				memcpy(dname, ns_rr_rdata(rr) + 1, n);
				printf("%s\ttext = \"%s\"\n", ns_rr_name(rr), dname);
			}
			break;

		case ns_t_soa:
			if (rdlen < 20) {
				printf("SOA record too short:%d\n", rdlen);
				return -1;
			}

			printf("%s\n", ns_rr_name(rr));

			cp = ns_rr_rdata(rr);
			n = ns_name_uncompress(ns_msg_base(handle), ns_msg_end(handle),
			                       cp, dname, sizeof(dname));
			if (n < 0) {
				//printf("Unable to uncompress domain: %s\n", strerror(errno));
				return -1;
			}

			printf("\torigin = %s\n", dname);
			cp += n;

			n = ns_name_uncompress(ns_msg_base(handle), ns_msg_end(handle),
			                       cp, dname, sizeof(dname));
			if (n < 0) {
				//printf("Unable to uncompress domain: %s\n", strerror(errno));
				return -1;
			}

			printf("\tmail addr = %s\n", dname);
			cp += n;

			printf("\tserial = %lu\n", ns_get32(cp));
			cp += 4;

			printf("\trefresh = %lu\n", ns_get32(cp));
			cp += 4;

			printf("\tretry = %lu\n", ns_get32(cp));
			cp += 4;

			printf("\texpire = %lu\n", ns_get32(cp));
			cp += 4;

			printf("\tminimum = %lu\n", ns_get32(cp));
			break;

		default:
			break;
		}
	}

	return i;
}

void nslookup_ipv6(const char *domain)
{
    unsigned char buf[65536];
    ns_msg handle;
    char ip[INET6_ADDRSTRLEN];
    int i=0;

    __open_nameservers();

    int len = res_query(domain, ns_c_in, ns_t_aaaa, buf, sizeof(buf));
    if (len < 0) {
printf("billy(%s# %d) Failed\n", __func__, __LINE__);
        fprintf(stderr, "Query failed: h_errno=%d\n", h_errno);
        return ;
    }

    printf("\nQuery succeeded, received %d bytes\n", len);
#if 0
    ns_initparse(buf, len, &handle);
    for (i = 0; i < ns_msg_count(handle, ns_s_an); i++) {
        ns_rr rr;
        ns_parserr(&handle, ns_s_an, i, &rr);
        if (ns_rr_type(rr) == ns_t_aaaa) {

            inet_ntop(AF_INET6, ns_rr_rdata(rr), ip, sizeof(ip));
            printf("IPv6: %s\n", ip);
        }
    }
#else
    parse_reply(buf, len);
#endif
    __close_nameservers();
    return ;
}

void nslookup_ipv4(const char *domain)
{
    unsigned char buf[65536] = {0};
    ns_msg handle;
    char ip[INET6_ADDRSTRLEN];
    int  i=0;
    int  len;

    __open_nameservers();

    len = res_query(domain, C_IN, T_A, buf, sizeof(buf));
    if (len < 0) {
        fprintf(stderr, "Query failed: h_errno=%d\n", h_errno);
        return ;
    }

    printf("\nQuery succeeded, received %d bytes\n", len);

#if 0
    ns_initparse(buf, len, &handle);
    for (i = 0; i < ns_msg_count(handle, ns_s_an); i++) {
        ns_rr rr;
        ns_parserr(&handle, ns_s_an, i, &rr);
        if (ns_rr_type(rr) == ns_t_a) {

            inet_ntop(AF_INET, ns_rr_rdata(rr), ip, sizeof(ip));
            printf("IPv4: %s\n", ip);
        }
    }

#else
    parse_reply(buf, len);
#endif
    __close_nameservers();
    return ;
}


int main(int argc, char *argv[])
{
    const char *domain;
    const char *dns_server = NULL;
    struct hostent *h;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <domain> [dns_server]\n", argv[0]);
        return 1;
    }

    domain = argv[1];
    if (argc == 3) {
        dns_server = argv[2];
        __open_nameservers_with_user_dns(dns_server);
    }


#if 1
    printf("Querying %s (IPv4)...\n", domain);
    h = gethostbyname2(domain, AF_INET);
    print_hostent(h);

    printf("Querying %s (IPv6)...\n", domain);
    h = gethostbyname2(domain, AF_INET6);
    print_hostent(h);

#else
    nslookup_ipv4(domain);
    nslookup_ipv6(domain);
#endif

    return 0;
}