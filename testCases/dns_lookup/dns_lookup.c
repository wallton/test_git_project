
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

void nslookup_ipv6(const char *domain) {
    struct addrinfo hints, *res, *p;
    int status;
    char ip_str[INET6_ADDRSTRLEN];

    // 初始化 hints 結構，指定查詢條件
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;      // 只查詢 IPv6 地址
    hints.ai_socktype = SOCK_STREAM; // 可選，指定協議類型

    // 執行域名解析
    status = getaddrinfo(domain, NULL, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return;
    }

    // 遍歷結果並顯示 IPv6 地址
    printf("IPv6 addresses for %s:\n", domain);
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        if (p->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            inet_ntop(AF_INET6, addr, ip_str, sizeof(ip_str));
            printf("  %s\n", ip_str);
        }
    }

    // 釋放記憶體
    freeaddrinfo(res);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <domain>\n", argv[0]);
        return 1;
    }

    nslookup_ipv6(argv[1]);
    return 0;
}

/*
IPv6 addresses for www.google.com:
  2607:f8b0:4004:834::2004
  2607:f8b0:4004:835::2004

*/