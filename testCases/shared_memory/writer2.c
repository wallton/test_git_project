#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "writer2_common.h"

#define SHM_NAME "/my_shared_memory"
//#define SHM_SIZE 1024
#define PcapShareTable (*_PcapShareTable)
#define pcap_free_queue (*_PcapShareTable).free_queue
#define pcap_allocated_queue (*_PcapShareTable).allocated_queue
#define pcap_send_queue (*_PcapShareTable).send_queue

#define ROOT_PATH "/home/billylin/yllib__mybase/test_git_project/testCases/shared_memory/"

tPcapShareMemory   *_PcapShareTable;
int shm_fd;
int shm_size;


unsigned int shm_init (void)
{

    // 創建共享記憶體物件
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    shm_size = sizeof(int) + sizeof(time_t)  + sizeof(int) + sizeof(tPcapQueueHeader)*3 + sizeof(tPcapEntry)*PCAP_PACKET_QUEUE_MAX;

printf("billy(%s# %d) shm_fd(%d) shm_size(%d)\n", __func__, __LINE__, shm_fd, shm_size);

    // 設置共享記憶體大小
    if (ftruncate(shm_fd, shm_size) == -1)
    {
        shm_unlink(SHM_NAME); 
        printf("%s %d ftruncate failed! %s\n", __func__, __LINE__, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* map shared memory file */
    _PcapShareTable = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (MAP_FAILED == _PcapShareTable)
    {
        printf("%s %d mmap failed! %s\n", __func__, __LINE__, strerror(errno));
        exit(EXIT_FAILURE);
    }

printf("billy(%s# %d) mmap table:%p\n", __func__, __LINE__, _PcapShareTable);
printf("\n\n");

    return (0);
}

unsigned int shm_cleanup (void)
{

    // 清理
    if (munmap((void *)_PcapShareTable, shm_size) == -1) {
        perror("munmap failed");
    }
    if (close(shm_fd) == -1) {
        perror("close failed");
    }
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink failed");
    }

    printf("Writer: Shared memory cleaned up.\n");

    return (0);
}


int main(int argc, char *argv[])
{
    int i=0;
    shm_init();



    // 寫入資料到共享記憶體
    PcapShareTable.free_queue.first = -1;
    PcapShareTable.free_queue.end = -1;
    PcapShareTable.free_queue.count = 0;

    PcapShareTable.allocated_queue.first = -1;
    PcapShareTable.allocated_queue.end = -1;
    PcapShareTable.allocated_queue.count = 0;

    PcapShareTable.send_queue.first = -1;
    PcapShareTable.send_queue.end = -1;
    PcapShareTable.send_queue.count = 0;

    for (i=0;i < PCAP_PACKET_QUEUE_MAX;i++)
    {
        memset(&(PcapShareTable.entryTable[i]), 0, sizeof(tPcapEntry));
        PcapShareTable.entryTable[i].next = -1;
    }

    for (i=0;i < PCAP_PACKET_QUEUE_MAX;i++)
    {
        SnPcapFreeNodeAdd(&i);
    }

#if 0
    const char *message = "Hello, this is shared memory!";
    strncpy(shm_ptr, message, SHM_SIZE - 1);
    ((char *)shm_ptr)[SHM_SIZE - 1] = '\0'; // 確保字串結尾

    printf("Writer: Data written to shared memory: %s\n", (char *)shm_ptr);
    printf("Writer: Waiting for reader (press Enter to exit)...\n");
    printf("Writer: shm_ptr: %p\n", shm_ptr);
#endif

    // 啟動 reader 程式作為獨立進程
    if (system( ROOT_PATH"reader2" ) == -1) {
        perror("system failed to start reader");
        munmap((void *)_PcapShareTable, shm_size);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    getchar(); // 等待用戶輸入以保持共享記憶體存在


    shm_cleanup();

    return 0;
}



