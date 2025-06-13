#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <errno.h>

#define UDP_SERVER "192.168.0.4" // 서버의 실제 IP 주소
#define S_PORT 5010             // 서버 포트 번호
#define BUF_SIZE 256 // 데이터 송수신 버퍼의 크기

int main() {
    int s; // 클라이언트 소켓 디스크립터
    struct sockaddr_in server_addr; // 서버 주소 정보
    struct sockaddr_in client_addr; // client 주소 정보
    char send_buf[BUF_SIZE]; // 클라이언트가 서버로 보낼 메시지를 저장할 버퍼
    char recv_buf[BUF_SIZE]; // 서버로부터 받은 응답을 저장할 버퍼
    ssize_t bytes_transferred; // 송수신된 바이트 수

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(UDP_SERVER);
    server_addr.sin_port = htons(S_PORT);
    // UDP 소켓 생성
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        perror("socket creation error"); exit(1);
    }
    printf("UDP client 소켓 생성\n");
    // client 주소 설정
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    client_addr.sin_port = htons(0);

    if (bind(s, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("client bind error");
        close(s); exit(1);
    }

    while (1) {
        printf("\n보낼 메시지 입력(quit 입력 시 종료): ");
        if (fgets(send_buf, sizeof(send_buf), stdin) == NULL) {
            perror("fgets error");
            break;
        }
        send_buf[strcspn(send_buf, "\n")] = '\0'; // 개행 문자 제거
        if (strcmp(send_buf, "quit") == 0) {
            printf("연결종료.\n");
            break;
        }
        //서버로 s소켓에 send_buf 담아서 보내기
        bytes_transferred = sendto(s, send_buf, strlen(send_buf) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        if (bytes_transferred < 0) {
            perror("sendto error"); close(s); exit(1);
        }

        memset(recv_buf, 0, BUF_SIZE);
        bytes_transferred = recvfrom(s, recv_buf, BUF_SIZE - 1, 0, (struct sockaddr *)NULL, NULL);
        if (bytes_transferred < 0) {
            perror("recvfrom error"); close(s); exit(1);
        }
        //서버에서 받은 메시지 출력
        recv_buf[bytes_transferred] = '\0';
        printf("%s\n", recv_buf);
    }
    close(s);
    return 0;
}