#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.0.4"
#define SERVER_PORT 5010
#define MAX_MSG_LEN 512

int main() {
    int c;
    struct sockaddr_in server_addr;
    char send_buf[MAX_MSG_LEN], recv_buf[MAX_MSG_LEN];
    ssize_t len;

    c = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(c, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        exit(1);
    }

    while (1) {
        printf("1. (학번 이름) 등록\n");
        printf("2. 학번 과목1 점수1 ... 등록\n");
        printf("3. 학번으로 이름찾기 (0 전체)\n");
        printf("4. 학번으로 성적찾기 (0 전체)\n");
        printf("5. 종료\n");

        printf("입력: ");
        fgets(send_buf, MAX_MSG_LEN, stdin);
        send_buf[strcspn(send_buf, "\n")] = '\0';  // 개행 제거

        int menu = atoi(send_buf);
        if (menu < 1 || menu > 5) {
            printf("잘못된 메뉴 번호입니다.\n");
            continue;
        }

        send(c, send_buf, strlen(send_buf) + 1, 0);
        if (menu == 5) break;

        len = recv(c, recv_buf, MAX_MSG_LEN - 1, 0);
        if (len <= 0) break;
        recv_buf[len] = '\0';
        printf("\n서버 응답완료\n\n%s\n", recv_buf);
    }

    close(c);
    return 0;
}
