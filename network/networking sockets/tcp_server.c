#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#define S_PORT 5010
#define MAX_MSG_LEN 1024
#define MAX_STUDENTS 20
#define MAX_NAME_LEN 20
#define MAX_COURSE_LEN 20
#define MAX_COURSES 4

typedef struct {
    char name[MAX_COURSE_LEN];
    int score;
} Course;

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    Course courses[MAX_COURSES];
    int registered_courses;
} Student;

Student students[MAX_STUDENTS];
int student_count = 0;

void client_process(int c);
int find_student_index(int id);
void reg_std_info(char* resp_buf, const char* send_data);
void reg_std_grade(char* resp_buf, const char* send_data);
void show_std_name(char* resp_buf, const char* send_data);
void show_std_grade(char* resp_buf, const char* send_data);

int main() {
    int s, c;
    struct sockaddr_in server, client;
    socklen_t len;

    s = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(S_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(s, (struct sockaddr*)&server, sizeof(server));
    listen(s, 5);

    printf("서버 PID : %d\n", getpid());
    while (1) {
        len = sizeof(client);
        c = accept(s, (struct sockaddr*)&client, &len);

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork error");
            close(c);
            continue;
        } else if (pid == 0) { // 자식 프로세스
            printf("자식 PID : %d\n", getpid());
            close(s); // 자식은 리스닝 소켓 닫음
            client_process(c);
            exit(0);
        } else { // 부모 프로세스
            close(c); // 부모는 클라이언트 통신 소켓 닫음
        }
    }
    close(s);
    return 0;
}

void client_process(int c) {
    char buf[MAX_MSG_LEN];
    char resp_buf[MAX_MSG_LEN];

    while (1) {
        memset(buf, 0, MAX_MSG_LEN);
        ssize_t bytes = recv(c, buf, MAX_MSG_LEN - 1, 0);
        if (bytes <= 0) break;

        int menu;
        if (sscanf(buf, "%d", &menu) != 1) {
            snprintf(resp_buf, MAX_MSG_LEN, "메뉴 번호 파싱 실패");
            send(c, resp_buf, strlen(resp_buf) + 1, 0);
            continue;
        }

        if (menu == 5) {
            printf("client 접속 종료 (PID : %d)\n",getpid());
            break;
        }
        char* data = strchr(buf, ' ');
        data = (data != NULL) ? data + 1 : "";

        switch (menu) {
            case 1: reg_std_info(resp_buf, data); break;
            case 2: reg_std_grade(resp_buf, data); break;
            case 3: show_std_name(resp_buf, data); break;
            case 4: show_std_grade(resp_buf, data); break;
            default:
                snprintf(resp_buf, MAX_MSG_LEN, "유효하지 않은 메뉴 번호입니다.");
                break;
        }

        send(c, resp_buf, strlen(resp_buf) + 1, 0);
    }

    close(c);
}

int find_student_index(int id) {
    for (int i = 0; i < student_count; i++) {
        if (students[i].id == id) return i;
    }
    return -1;
}
// 1번
void reg_std_info(char* resp_buf, const char* send_data) {
    int id;
    char name[MAX_NAME_LEN];
    if (sscanf(send_data, "%d %s", &id, name) != 2) {
        snprintf(resp_buf, MAX_MSG_LEN, "형식 오류: <학번> <이름>"); return;
    }
    if (student_count >= MAX_STUDENTS) {
        snprintf(resp_buf, MAX_MSG_LEN, "최대 학생 수 초과"); return;
    }
    if (find_student_index(id) != -1) {
        snprintf(resp_buf, MAX_MSG_LEN, "이미 등록된 학번입니다"); return;
    }
    students[student_count].id = id;
    strncpy(students[student_count].name, name, MAX_NAME_LEN);
    students[student_count].registered_courses = 0;
    student_count++;
    printf("등록 완료: %d %s\n", id, name);
    snprintf(resp_buf, MAX_MSG_LEN, "등록 완료: %d %s", id, name);
}
//2번
void reg_std_grade(char* resp_buf, const char* send_data) {
    int id, score, parsed_len, i = 0;
    char course[MAX_COURSE_LEN];
    char* ptr = (char*)send_data;
    if (sscanf(ptr, "%d%n", &id, &parsed_len) != 1) {
        snprintf(resp_buf, MAX_MSG_LEN, "형식 오류: <학번> <과목> <성적>..."); return;
    }
    ptr += parsed_len;
    int idx = find_student_index(id);
    if (idx == -1) {
        snprintf(resp_buf, MAX_MSG_LEN, "학번 %d 없음", id); return;
    }
    students[idx].registered_courses = 0;
    while (sscanf(ptr, "%s %d%n", course, &score, &parsed_len) == 2 && i < MAX_COURSES) {
        strncpy(students[idx].courses[i].name, course, MAX_COURSE_LEN);
        students[idx].courses[i].score = score;
        students[idx].registered_courses++;
        i++;
        ptr += parsed_len;
    }
    printf("%d번 학생 성적 등록 완료 (%d과목)\n", id, i);
    snprintf(resp_buf, MAX_MSG_LEN, "%d번 학생 성적 등록 완료 (%d과목)", id, i);
}
//3번
void show_std_name(char* resp_buf, const char* send_data) {
    int id;
    if (sscanf(send_data, "%d", &id) != 1) {
        snprintf(resp_buf, MAX_MSG_LEN, "형식 오류: <학번> (0 전체)"); return;
    }
    int len = 0;
    if (id == 0) {
        for (int i = 0; i < student_count; i++) {
            len += snprintf(resp_buf + len, MAX_MSG_LEN - len, "%d: %s\n", students[i].id, students[i].name);
        }
        printf("3. (0 전체) 조회완료\n");
    } else {
        int idx = find_student_index(id);
        if (idx == -1) {
            snprintf(resp_buf, MAX_MSG_LEN, "해당 학번 없음"); return;
        }
        printf("3. 조회완료\n");
        snprintf(resp_buf, MAX_MSG_LEN, "%d: %s", students[idx].id, students[idx].name);
    }
}
//4번
void show_std_grade(char* resp_buf, const char* send_data) {
    int id;
    if (sscanf(send_data, "%d", &id) != 1) {
        snprintf(resp_buf, MAX_MSG_LEN, "형식 오류: <학번> (0 전체)"); return;
    }
    int len = 0;
    if (id == 0) {
        for (int i = 0; i < student_count; i++) {
            len += snprintf(resp_buf + len, MAX_MSG_LEN - len, "%d (%s) : \n", students[i].id, students[i].name);
            for (int j = 0; j < students[i].registered_courses; j++) {
                len += snprintf(resp_buf + len, MAX_MSG_LEN - len, "(%s : %d)\n", students[i].courses[j].name, students[i].courses[j].score);
            }
        }
        printf("4. (0 전체) 조회완료\n");
    } else {
        int idx = find_student_index(id);
        if (idx == -1) {
            snprintf(resp_buf, MAX_MSG_LEN, "해당 학번 없음"); return;
        }
        len += snprintf(resp_buf + len, MAX_MSG_LEN - len, "%d (%s) -> \n", students[idx].id, students[idx].name);
        for (int j = 0; j < students[idx].registered_courses; j++) {
            len += snprintf(resp_buf + len, MAX_MSG_LEN - len, "(%s : %d)  \n", students[idx].courses[j].name, students[idx].courses[j].score);
        }
        printf("4. 조회완료\n");
    }
}
