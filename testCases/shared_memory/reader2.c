#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "writer2_common.h"

#define SHM_NAME "/my_shared_memory"
//#define SHM_SIZE 1024

int main(int argc, char *argv[])
{
    int shm_size = sizeof(int) + sizeof(time_t)  + sizeof(int) + sizeof(tPcapQueueHeader)*3 + sizeof(tPcapEntry)*PCAP_PACKET_QUEUE_MAX;

    // 打開共享記憶體物件
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // 映射共享記憶體
    void *shm_ptr = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    tPcapShareMemory   *_pcap_ptr = shm_ptr;


    // 讀取共享記憶體中的資料
    //printf("Reader: Data read from shared memory: %s\n", (char *)shm_ptr);
    //printf("Reader: shm_ptr: %p\n", shm_ptr);

    printf("Reader: free_queue.first : %d\n", (*_pcap_ptr).free_queue.first);
    printf("Reader: free_queue.end : %d\n", (*_pcap_ptr).free_queue.end);
    printf("Reader: free_queue.count : %d\n", (*_pcap_ptr).free_queue.count);

    printf("Reader: allocated_queue.first : %d\n", (*_pcap_ptr).allocated_queue.first);
    printf("Reader: allocated_queue.end : %d\n", (*_pcap_ptr).allocated_queue.end);
    printf("Reader: allocated_queue.count : %d\n", (*_pcap_ptr).allocated_queue.count);

    printf("Reader: send_queue.first : %d\n", (*_pcap_ptr).send_queue.first);
    printf("Reader: send_queue.end : %d\n", (*_pcap_ptr).send_queue.end);
    printf("Reader: send_queue.count : %d\n", (*_pcap_ptr).send_queue.count);


    // 清理
    if (munmap(shm_ptr, shm_size) == -1) {
        perror("munmap failed");
    }
    if (close(shm_fd) == -1) {
        perror("close failed");
    }

    return 0;
}