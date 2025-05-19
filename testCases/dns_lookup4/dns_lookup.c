
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>



void print_hostent(struct hostent *host)
{
    char ip[INET6_ADDRSTRLEN] = {0};
    char **alias = NULL;
    char **addr = NULL;


    if (host == NULL) {
        fprintf(stderr, "Error: %s\n", hstrerror(h_errno));
        return;
    }

    printf("Official name: %s\n", host->h_name);
    for (alias = host->h_aliases; *alias != NULL; alias++)
    {
        printf("Alias: %s\n", *alias);
    }


    for (addr = host->h_addr_list; *addr != NULL; addr++)
    {
        inet_ntop(host->h_addrtype, *addr, ip, INET6_ADDRSTRLEN);
        printf("IP Address: %s\n", ip);
    }
}

/* USE gethostbyname */
void nslookup (const char *domain)
{
    char ip[INET_ADDRSTRLEN] = {0};
    struct hostent *host = gethostbyname(domain);
    char **alias = NULL;
    char **addr = NULL;


    if (host == NULL) {
        fprintf(stderr, "Error: %s\n", hstrerror(h_errno));
        return ;
    }

    printf("Official name: %s\n", host->h_name);
    for (alias = host->h_aliases; *alias != NULL; alias++)
    {
        printf("Alias: %s\n", *alias);
    }

    for (addr = host->h_addr_list; *addr != NULL; addr++)
    {    
        inet_ntop(AF_INET, *addr, ip, INET_ADDRSTRLEN);
        printf("IP Address: %s\n", ip);
    }
    
}

int main(int argc, char *argv[])
{
    const char *hostname = NULL;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <domain>\n", argv[0]);
        return 1;
    }

    hostname = argv[1];


    // IPv4 查詢
    printf("Querying IPv4...\n");
    struct hostent *host_ipv4 = gethostbyname2(hostname, AF_INET);
    print_hostent(host_ipv4);

    // IPv6 查詢
    printf("\nQuerying IPv6...\n");
    struct hostent *host_ipv6 = gethostbyname2(hostname, AF_INET6);
    print_hostent(host_ipv6);

    return 0;
}
/***

$ ./testCases/dns_lookup4/test google.com
Querying IPv4...
Official name: google.com
IP Address: 142.250.204.46

Querying IPv6...
Official name: google.com
IP Address: 2404:6800:4012:9::200e


***/


/***

$ ./testCases/dns_lookup4/test iotacs.jioconnect.com
Querying IPv4...
Official name: iotacs.jioconnect.com
IP Address: 49.40.14.88

Querying IPv6...
Official name: iotacs.jioconnect.com
IP Address: 2405:200:1601:c2e0:49:40:14:88

***/














