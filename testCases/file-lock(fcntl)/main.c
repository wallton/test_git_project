#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/*** --------------------------------------------------***/


int lock_file(int fd, short type) {
    struct flock fl;
    fl.l_type = type;         // F_RDLCK（共享鎖）或 F_WRLCK（排他鎖）
    fl.l_whence = SEEK_SET;   // 鎖從檔案開頭開始
    fl.l_start = 0;           // 鎖的起始位元組
    fl.l_len = 0;             // 0 表示鎖整個檔案
    fl.l_pid = getpid();

    return fcntl(fd, F_SETLKW, &fl);  // 使用 F_SETLKW（阻塞式鎖）
}

int unlock_file(int fd) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    return fcntl(fd, F_SETLK, &fl);
}



/*** --------------------------------------------------***/

int main()
{
    int fd = open("test.txt", O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        perror("lock_file");
        close(fd);
        return 1;
    }

    printf("File locked. Press Enter to unlock...\n");
    getchar();

    if (unlock_file(fd) == -1) {
        perror("unlock_file");
    }

    close(fd);
    return 0;
}