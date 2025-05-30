#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define FILE_NAME "shared_file.txt"
#define MAX_WRITES 10
#define ROOT_PATH "/home/billylin/yllib__mybase/test_git_project/testCases/flock-LOCK_SH/"

int main(int argc, char *argv[])
{
    if (access(FILE_NAME, F_OK) == 0)
    {
        // file exists
        if (unlink(FILE_NAME) == -1)
        {
            perror("unlink failed");
        }
    }


    int fd = open(FILE_NAME, O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    printf("Writer: Starting reader_sh...\n");
    if (system( ROOT_PATH"reader_sh 1") == -1) {
        perror("system failed to start reader_sh");
        exit(EXIT_FAILURE);
    }
    if (system( ROOT_PATH"reader_sh 2") == -1) {
        perror("system failed to start reader_sh");
        exit(EXIT_FAILURE);
    }
    if (system( ROOT_PATH"reader_sh 3") == -1) {
        perror("system failed to start reader_sh");
        exit(EXIT_FAILURE);
    }

    printf("\n\nWriter: Starting writing............................\n\n\n");

    for (int i = 1; i <= MAX_WRITES; i++)
    {
        printf("Writer: Attempting to acquire exclusive lock (write %d)...\n", i);

        if (flock(fd, LOCK_EX) == -1) {
            perror("flock LOCK_EX failed");
            close(fd);
            exit(EXIT_FAILURE);
        }


        time_t now = time(NULL);
        char *time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0';

        // 寫入訊息
        char message[256];
        snprintf(message, sizeof(message), "msg[%s] Write %d: Exclusive write\n", time_str, i);
        if (write(fd, message, strlen(message)) == -1) {
            perror("write failed");
            flock(fd, LOCK_UN);
            close(fd);
            exit(EXIT_FAILURE);
        }

        printf("Writer: Written: %s", message);
        printf("Writer: Holding lock for 5 seconds...\n");
        sleep(5);

        // 釋放鎖
        if (flock(fd, LOCK_UN) == -1) {
            perror("flock unlock failed");
            close(fd);
            exit(EXIT_FAILURE);
        }

        printf("Writer: Released lock (write %d).\n", i);
        if (i < MAX_WRITES) {
            printf("Writer: Waiting 5 seconds before next write...\n");
            sleep(5);
        }
        printf("\n\n");
    }

    if (close(fd) == -1) {
        perror("close failed");
        exit(EXIT_FAILURE);
    }




    printf("Writer: Started reader_sh and exiting.\n");
    return 0;
}