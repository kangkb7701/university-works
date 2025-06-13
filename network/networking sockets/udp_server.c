#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h> 
#include <time.h>
#include <stdio.h>   
#include <unistd.h>
#include <arpa/inet.h>

#define S_PORT 5010
// client 구조체(client 정보를 따로 저장해야 구분가능)
typedef struct ClientInfo {
    struct sockaddr_in addr;
    int message_count;
    struct ClientInfo *next; // 다음 클라이언트를 가리키는 포인터 (연결 리스트용)
} ClientInfo;

// 클라이언트 연결 리스트의 헤드
ClientInfo *client_list = NULL;
ClientInfo* find_or_create_client(struct sockaddr_in *new_client_addr);

int main() { 
    int s, c;
    struct sockaddr_in server, client; 
    socklen_t client_len; // 클라이언트 주소 구조체의 길이 (recvfrom에서 필요)
    char buf[256]; // 데이터 수신 버퍼
    int buf_len; // 수신된 데이터 길이
    char sendbuf[512]; //데이터 송신 버퍼
    time_t today;
    char time_str[64];

    // 1. 소켓 생성 : s = sokcet describtor 반환환
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket creation error"); 
        exit(1);
    }

    // 2. 서버 주소 구조체 설정
    server.sin_family = AF_INET;                  /*AF_INET*/
    server.sin_port = htons(S_PORT);         /*포트번호 : 5010*/
    server.sin_addr.s_addr = htonl(INADDR_ANY);   /*htonl() : IP정수값 해석할 수 있게 변환환*/
    // bind() : 생성된 소켓(s)에 주소 부여, 소켓에 IP주소, 포트번호 연결
    if (bind(s, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("bind error"); close(s); exit(1);
    }
    printf("서버 포트: %d\n", S_PORT);

    while (1) {
        client_len = sizeof(client); 
        memset(buf, 0, sizeof(buf));
        memset(sendbuf, 0, sizeof(buf));
        // 클라이언트로부터 데이터 수신 (recvfrom)
        buf_len = recvfrom(s, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&client, &client_len);

        if (buf_len < 0) { // recvfrom 오류 발생 시
            perror("recvfrom error"); continue;
        }
        buf[buf_len] = '\0'; // 수신된 데이터 끝에 널 문자 추가
        // 클라이언트 정보 찾기 또는 생성   
        ClientInfo *current_client = find_or_create_client(&client);
        if (current_client == NULL) { //클라이언트 정보를 가져오지 못한 경우
            fprintf(stderr, "클라이언트 정보 처리 오류\n"); continue;
        }
        current_client->message_count++; //n번째 메시지 count
        printf("받은 메시지 : %s, client 정보 : %s:%d\n", buf, inet_ntoa(current_client->addr.sin_addr), 
                        ntohs(current_client->addr.sin_port)); // 수신된 메시지와 클라이언트 정보 출력
        // 현재 시간 얻기
        time(&today);
        strncpy(time_str, ctime(&today), 24);
        time_str[24] = '\0';
        //sendbuf에 보낼 메시지 담기기
        snprintf(sendbuf, sizeof(sendbuf),
            "i received %dth message from client %s at %s : %s",
            current_client->message_count,
            inet_ntoa(current_client->addr.sin_addr), // 클라이언트 IP 주소
            time_str, buf);    // 서버가 수신한 시간, 클라이언트 메시지 내용
        //메시지 client한테 전달
        sendto(s, sendbuf, strlen(sendbuf) + 1, 0, (struct sockaddr *)&client, client_len);
    }
    close(s); // 서버 종료 시 소켓 닫기
    return 0; // main 함수의 반환값
}

ClientInfo* find_or_create_client(struct sockaddr_in *new_client_addr) {
    ClientInfo *current = client_list;
    // 기존 클라이언트 리스트에서 찾기
    while (current != NULL) { // IP 주소와 포트 번호가 모두 일치하는 client가 있으면,
        if (current->addr.sin_addr.s_addr == new_client_addr->sin_addr.s_addr && current->addr.sin_port == new_client_addr->sin_port) {  
            return current; // 해당 클라이언트 정보 반환
        }
        current = current->next;
    }
    // 못 찾았으면 새로운 클라이언트 정보 생성
    ClientInfo *new_client = (ClientInfo *)malloc(sizeof(ClientInfo));
    if (new_client == NULL) {
        perror("새 client 할당 실패"); exit(1);
    }
    memcpy(&(new_client->addr), new_client_addr, sizeof(struct sockaddr_in)); // 주소 정보 복사
    new_client->message_count = 0; // 새 클라이언트이므로 카운트 0으로 초기화
    new_client->next = client_list; // 리스트의 맨 앞에 추가
    client_list = new_client;

    printf("새로운 클라이언트 연결: %s:%d\n", inet_ntoa(new_client_addr->sin_addr), ntohs(new_client_addr->sin_port));
    return new_client;
}
