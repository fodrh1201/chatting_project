#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE	1024
#define INT_SIZE	4

void error_handling(char* message);

void* thread_write(void *arg);
void* thread_read(void *arg);

int sock;

int main(int argc, char* argv[]) {
	int write_cnt, write_len, recv_cnt, recv_len;

	char message[BUF_SIZE];
	struct sockaddr_in serv_addr;

	pthread_t tid[2];
	int tret;

	if (argc != 3) {
		printf("Usage : %s <IP> <port> \n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error!");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	
	printf("%s\n", argv[1]);

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error!");
	else
		puts("Connected....... ");

	tret = pthread_create(&tid[0], 0, thread_write, (void *) 1);
	if (tret != 0)
		error_handling("write_thread() create error!");

	tret = pthread_create(&tid[1], 0, thread_read, (void *) 2);
	if (tret != 0)
		error_handling("read_thread() create error!");

	int i;
	for (i = 0; i < 2; i++) {
		tret = pthread_join(tid[i], 0);
		if (tret != 0)
			error_handling("join error!");
	}

	close(sock);
	return 0;
}
	
	
void *thread_read(void *arg) {
	char buf[BUF_SIZE];
	int read_cnt, read_len, recv_len;

	while (strcmp(buf, "exit")) {
		read_cnt = 0;
		read(sock, (char*)&recv_len, 1);
		while(recv_len) {
			read_cnt = read(sock, buf + read_len, BUF_SIZE);
			if (read_cnt = -1)
				error_handling("read() error!");
			recv_len -= read_cnt;
			read_len += read_len;
		}
		printf("Other> %s\n", buf);
	}
	pthread_exit((void *) NULL);
}

void *thread_write(void *arg) {
	char buf[BUF_SIZE];
	int write_cnt, write_len, send_len;
	char len;

	while (strcmp(buf, "exit")) {
		printf("Me> ");
		scanf("%s", buf);
		send_len = strlen(buf);
		len = (char)send_len;
		write(sock, (char*)&len, 1);
		while (send_len) {
			write_cnt = write(sock, buf + write_len, BUF_SIZE);
			if (write_cnt == -1)
				error_handling("write() error!");
			send_len -= write_cnt;
			write_len += write_cnt;
		}
	}
	pthread_exit((void*) NULL);
}
void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
