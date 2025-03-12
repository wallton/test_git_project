#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>

/*** --------------------------------------------------***/
#define MAX_LINE 256
#define MAX_HOPS 30

// 儲存每跳的資訊
struct Hop {
    int hop_number;          // 跳數
    char address[INET6_ADDRSTRLEN]; // IPv6 地址或域名
    float rtt[3];            // 三次 RTT（毫秒）
    int rtt_count;           // 有效 RTT 數量（可能少於 3）
};

// 解析一行 traceroute6 輸出
int parse_hop_line(char *line, struct Hop *hop)
{
    char *token;
    int rtt_index = 0;

    // remove newline
    line[strcspn(line, "\n")] = 0;

    token = strtok(line, " ");
    if (!token || !isdigit(token[0])) return 0; // invalid lines（非跳數開頭）

    // 提取跳數
    hop->hop_number = atoi(token);

    // 提取地址
    token = strtok(NULL, " ");
    if (!token) return 0; // 無地址
    if (strcmp(token, "*") == 0) {
        strcpy(hop->address, "*"); // 未回應的情況
    } else {
        strncpy(hop->address, token, INET6_ADDRSTRLEN - 1);
        hop->address[INET6_ADDRSTRLEN - 1] = '\0';
    }

    // 初始化 RTT
    hop->rtt_count = 0;
    for (int i = 0; i < 3; i++) hop->rtt[i] = -1.0;

    // 提取 RTT
    while ((token = strtok(NULL, " ")) != NULL && rtt_index < 3) {
        if (strcmp(token, "*") == 0) {
            hop->rtt[rtt_index++] = -1.0; // 未回應記為 -1
        } else if (strstr(token, "ms")) {
            hop->rtt[rtt_index] = atof(token); // 轉換為浮點數
            rtt_index++;
        }
    }
    hop->rtt_count = rtt_index;

    return 1; // 成功解析
}


/*** --------------------------------------------------***/

int main(int argc, char *argv[])
{
    char line[MAX_LINE];
    struct Hop hops[MAX_HOPS];
    int hop_count = 0;

    // 從標準輸入讀取 traceroute6 輸出（可改為文件）
    printf("請輸入 traceroute6 輸出（每行一跳，按 Ctrl+D 結束）:\n");
    while (fgets(line, sizeof(line), stdin)) {
        // 跳過第一行（traceroute to ...）
        if (strstr(line, "traceroute to")) continue;

        // 解析每一跳
        if (hop_count < MAX_HOPS && parse_hop_line(line, &hops[hop_count])) {
            hop_count++;
        }
    }

    // 顯示解析結果
    printf("\n解析結果:\n");
    for (int i = 0; i < hop_count; i++) {
        printf("Hop %d:\n", hops[i].hop_number);
        printf("  Address: %s\n", hops[i].address);
        printf("  RTT: ");
        for (int j = 0; j < hops[i].rtt_count; j++) {
            if (hops[i].rtt[j] >= 0) {
                printf("%.3f ms ", hops[i].rtt[j]);
            } else {
                printf("* ");
            }
        }
        printf("\n");
    }

    return 0;
}