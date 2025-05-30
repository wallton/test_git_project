#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*** --------------------------------------------------***/

/*** --------------------------------------------------***/

int main(int argc, char *argv[]) 
{
    char *command = "ls /tmp"; // 範例：列出 /tmp 目錄內容
                               //const char *command = "ls /nonexistent"; // 測試失敗情況


    if (argc > 1) {
        command = argv[1];
    }

    // 使用 popen 執行命令，模式為 "r"（讀取輸出）
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "popen 失敗\n");
        return 1;
    }

    // 可選：讀取命令輸出
#if 0
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("命令輸出: %s", buffer);
    }
#endif
    // 關閉 popen 並獲取退出狀態
    int status = pclose(fp);
    if (status == -1) {
        fprintf(stderr, "pclose 失敗\n");
        return 1;
    }

    // 檢查命令退出碼
    if (WIFEXITED(status)) { // 確認命令正常退出
        int exit_code = WEXITSTATUS(status); // 取得退出碼
        if (exit_code == 0) {
            printf("命令執行成功 (退出碼: %d)\n", exit_code);
        } else {
            printf("命令執行失敗 (退出碼: %d)\n", exit_code);
        }
    } else {
        printf("命令異常終止\n");
    }

    return 0;
}


