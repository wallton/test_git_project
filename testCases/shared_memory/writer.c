#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 1024


int main()
{
    // 創建共享記憶體物件
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // 設置共享記憶體大小
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate failed");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // 映射共享記憶體
    void *shm_ptr = mmap(NULL, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // 寫入資料到共享記憶體
    const char *message = "Hello, this is shared memory!";
    strncpy(shm_ptr, message, SHM_SIZE - 1);
    ((char *)shm_ptr)[SHM_SIZE - 1] = '\0'; // 確保字串結尾

    printf("Writer: Data written to shared memory: %s\n", (char *)shm_ptr);
    printf("Writer: Waiting for reader (press Enter to exit)...\n");
    printf("Writer: shm_ptr: %p\n", shm_ptr);

    // 啟動 reader 程式作為獨立進程
    if (system("./reader") == -1) {
        perror("system failed to start reader");
        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    getchar(); // 等待用戶輸入以保持共享記憶體存在



    // 清理
    if (munmap(shm_ptr, SHM_SIZE) == -1) {
        perror("munmap failed");
    }
    if (close(shm_fd) == -1) {
        perror("close failed");
    }
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink failed");
    }

    printf("Writer: Shared memory cleaned up.\n");
    return 0;
}


