#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <resolv.h>
#include <arpa/inet.h>


void nslookup_ipv6(const char *domain)
{
    unsigned char buf[65536];
    ns_msg handle;
    char ip[INET6_ADDRSTRLEN];


    res_init();
    int len = res_query(domain, ns_c_in, ns_t_aaaa, buf, sizeof(buf));
    if (len < 0) {
        perror("res_query AAAA");
        return ;
    }

printf("billy(%s# %d) buf(len=%d)\n(%s)\n", __func__, __LINE__, len, buf);

    ns_initparse(buf, len, &handle);
    for (int i = 0; i < ns_msg_count(handle, ns_s_an); i++) {
        ns_rr rr;
        ns_parserr(&handle, ns_s_an, i, &rr);
        if (ns_rr_type(rr) == ns_t_aaaa) {

            inet_ntop(AF_INET6, ns_rr_rdata(rr), ip, sizeof(ip));
            printf("IPv6: %s\n", ip);
        }
    }

    return ;
}

void nslookup_ipv4(const char *domain)
{
    unsigned char buf[65536];
    ns_msg handle;
    char ip[INET6_ADDRSTRLEN];


    res_init();
    int len = res_query(domain, ns_c_in, ns_t_a, buf, sizeof(buf));
    if (len < 0) {
        perror("res_query A return-failure");
        return ;
    }

printf("billy(%s# %d) buf(len=%d)\n(%s)\n", __func__, __LINE__, len, buf);

    ns_initparse(buf, len, &handle);
    for (int i = 0; i < ns_msg_count(handle, ns_s_an); i++) {
        ns_rr rr;
        ns_parserr(&handle, ns_s_an, i, &rr);
        if (ns_rr_type(rr) == ns_t_a) {

            inet_ntop(AF_INET, ns_rr_rdata(rr), ip, sizeof(ip));
            printf("IPv4: %s\n", ip);
        }
    }

    return ;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <domain>\n", argv[0]);
        return 1;
    }

    nslookup_ipv4(argv[1]);
    nslookup_ipv6(argv[1]);
    return 0;
}

/*
$ ./test acs.oss.jio.com
IPv4: 49.40.8.93
IPv6: 2405:200:1601:c0e1:49:40:8:93


*/