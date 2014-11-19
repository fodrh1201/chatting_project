#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>

#define BUF_SIZE	1024
#define INT_SIZE	4

void error_handling(char* message);

void* thread_write(void *arg);
void* thread_read(void *arg);

int sock;
char name[BUF_SIZE];
int name_len;

int main(int argc, char* argv[]) {
	int i;
	int sock;
	struct sockaddr_in serv_adr;

	pthread_t tid[2];
	int tret;

	if (argc != 3) {
		printf("Usage : %s <IP> <port> \n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error!");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("connect() error!");
	else
		puts("Connected........");


	printf("Input your name : ");
	scanf("%s", name);	
	name_len = strlen(name);

	tret = pthread_create(&tid[0], 0, thread_write, NULL);
	if (tret != 0)
		error_handling("write_thread() create error!");

	tret = pthread_create(&tid[1], 0, thread_read, NULL);
	if (tret != 0)
		error_handling("read_thread() create error!");

	for (i = 0; i < 2; i++) {
		tret = pthread_join(tid[i], 0);
		if (tret != 0)
			error_handling("join error!");
	}

	close(sock);
	return 0;
}

void *thread_read(void *arg) {
	int u_len, t_len;
	int write_cnt, write_len, read_cnt, read_len;

	char user_name[BUF_SIZE];
	char message[BUF_SIZE];

	while (1) {
		read(sock, (char*)&u_len, 1);
		read_len = 0;
		while (read_len < u_len) {
			read_cnt = read(sock, user_name + read_len, u_len - read_len);
			if (read_cnt == -1)
				error_handling("name read() error!");
			read_len += read_cnt;
		}

		read(sock, (char*)&t_len, 1);
		read_len = 0;
		while (read_len < t_len) {
			read_cnt = read(sock, message + read_len, t_len - read_len);
			if (read_cnt == -1)
				error_handling("message read() error!");
			read_len += read_cnt;
		}
		printf("%s > %s\n", user_name, message);
	}
	pthread_exit((void*) NULL);
}

void *thread_write(void *arg) {
	char buf[BUF_SIZE];
	int t_len;
	
	int write_cnt, write_len;
	
	while (1) {
		char temp = (char)name_len;
		write(sock, &temp, 1);
		printf("ME> ");
		fgets(buf, BUF_SIZE, stdin);
		t_len = strlen(buf);
		
		printf("%d, %d\n", name_len, t_len);

		write_len = 0;
		while (write_len < name_len) {
			write_cnt = write(sock, name + write_len, name_len - write_len);
			if (write_cnt == -1)
				error_handling("wirte name error!");
			write_len += write_cnt;
		}

		temp = (char)t_len;
		write(sock, &temp, 1);
		write_len = 0;
		while (write_len < t_len) {
			write_cnt = write(sock, buf + write_len, t_len - write_len);
			if (write_cnt == -1)
				error_handling("write text error!");
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


