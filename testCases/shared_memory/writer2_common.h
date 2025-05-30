#ifndef _PCAP_QUEUE_H
#define _PCAP_QUEUE_H


/* "libpcap" record header. */
struct pcap_hdr {
    unsigned int magic;            /* magic number */
    unsigned short version_major;  /* major version number */
    unsigned short version_minor;  /* minor version number */
    int  thiszone;               /* GMT to local correction */
    unsigned int sigfigs;        /* accuracy of timestamps */
    unsigned int snaplen;        /* max length of captured packets, in octets */
    unsigned int network;        /* data link type */
};

struct pcaprec_hdr {
    unsigned int ts_sec;         /* timestamp seconds */
    unsigned int ts_usec;        /* timestamp microseconds (nsecs for PCAP_NSEC_MAGIC) */
    unsigned int incl_len;       /* number of octets of packet saved in file */
    unsigned int orig_len;       /* actual length of packet */
};

#define SUCCESS 0
#define FAILURE 1
#define PCAP_PACKET_MAX_LENGTH      1522

typedef struct {
    int next;
    struct pcaprec_hdr rec_hdr;
    unsigned char u1PacketBuffer[PCAP_PACKET_MAX_LENGTH];
} tPcapEntry;

typedef struct {
    int          first;
    int          end;
    unsigned int count;
} tPcapQueueHeader;

#ifdef SN_PCAP_PACKET_QUEUE_MAX
#define   PCAP_PACKET_QUEUE_MAX      SN_PCAP_PACKET_QUEUE_MAX
#else
#define   PCAP_PACKET_QUEUE_MAX      800
#endif

typedef struct {
    int    current_node;
    time_t duration; /* second */
    int    lock_fp;
    tPcapQueueHeader free_queue;
    tPcapQueueHeader allocated_queue;
    tPcapQueueHeader send_queue;
    tPcapEntry entryTable[PCAP_PACKET_QUEUE_MAX];
} tPcapShareMemory;



int SnPcapFreeNodeGet (int *);
int SnPcapFreeNodeAdd (int *);
int SnPcapFreeNodeRemove (int *);

int SnPcapUsedNodeGet (int *);
int SnPcapUsedNodeAdd (int *);
int SnPcapUsedNodeRemove (int *);

int SnPcapSenderNodeGet (int *);
int SnPcapSenderNodeAdd (int *);
int SnPcapSenderNodeRemove (int *);


#endif /* _PCAP_QUEUE_H */




