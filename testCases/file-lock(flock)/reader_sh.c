#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>

#define FILE_NAME "shared_file.txt"
#define BUFFER_SIZE 1024
#define MAX_READS 3

int main(int argc, char *argv[])
{
    int readr_no = -1;
    
    if (argc > 1)
    {
        readr_no = atoi(argv[1]);
    }
    

    for (int i = 1; i <= MAX_READS; i++) {
        // 打開檔案
        int fd = open(FILE_NAME, O_RDONLY);
        if (fd == -1) {
            perror("open failed");
            exit(EXIT_FAILURE);
        }

        // 獲取共享鎖
        printf("Reader[%d]: Attempting to acquire shared lock (read %d)...\n", readr_no, i);
        time_t attempt_time = time(NULL);
        char *attempt_time_str = ctime(&attempt_time);
        attempt_time_str[strlen(attempt_time_str) - 1] = '\0';

        if (flock(fd, LOCK_SH) == -1) {
            perror("flock LOCK_SH failed");
            close(fd);
            exit(EXIT_FAILURE);
        }

        // 計算阻塞時間
        time_t lock_time = time(NULL);
        char *lock_time_str = ctime(&lock_time);
        lock_time_str[strlen(lock_time_str) - 1] = '\0';
        printf("Reader[%d]: Acquired shared lock at %s (waited %ld seconds).\n", 
               readr_no, lock_time_str, lock_time - attempt_time);

        // 讀取檔案
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("read failed");
            flock(fd, LOCK_UN);
            close(fd);
            exit(EXIT_FAILURE);
        }
        buffer[bytes_read] = '\0';

        printf("Reader[%d]: Data read: %s", readr_no, buffer);
        printf("Reader[%d]: Holding lock for 5 seconds...\n", readr_no);
        sleep(1);

        // 釋放鎖
        if (flock(fd, LOCK_UN) == -1) {
            perror("flock unlock failed");
            close(fd);
            exit(EXIT_FAILURE);
        }

        printf("Reader[%d]: Released lock (read %d).\n", readr_no, i);
        if (close(fd) == -1) {
            perror("close failed");
            exit(EXIT_FAILURE);
        }

        if (i < MAX_READS) {
            printf("Reader[%d]: Waiting 5 seconds before next read...\n", readr_no);
            sleep(25);
        }
    }

    /*
    if (unlink(FILE_NAME) == -1) {
        perror("unlink failed");
    } else {
        printf("Reader: Shared file cleaned up.\n");
    }
    */

    return 0;
}