#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 1024

int main()
{
    // 打開共享記憶體物件
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // 映射共享記憶體
    void *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    // 讀取共享記憶體中的資料
    printf("Reader: Data read from shared memory: %s\n", (char *)shm_ptr);
    printf("Reader: shm_ptr: %p\n", shm_ptr);

    // 清理
    if (munmap(shm_ptr, SHM_SIZE) == -1) {
        perror("munmap failed");
    }
    if (close(shm_fd) == -1) {
        perror("close failed");
    }

    return 0;
}