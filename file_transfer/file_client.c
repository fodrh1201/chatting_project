#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE	1024
#define INT_SIZE	4
#define FILE_SIZE  	1677721

void error_handling(char* message);

int main(int argc, char* argv[]) {
	int sock;
	int write_cnt, write_len, recv_cnt, recv_len;
	int file_len, name_len;
	FILE* file;

	char file_name[BUF_SIZE];
	char file_buf[FILE_SIZE];
	struct sockaddr_in serv_addr;

	if (argc != 2) {
		printf("Usage : %s <port> \n", argv[0]);
		exit(1);
	}
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error!");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(atoi(argv[1]));

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error!");
	else
		puts("Connected.............");

	printf("Type file name : ");
	fgets(&file_name[1], BUF_SIZE-1, stdin);
	name_len = strlen(&file_name[1]);
	file_name[0] = (char)name_len;

	write(sock, file_name, strlen(file_name));
	
	recv_len = 0;
	while (recv_len < INT_SIZE) {
		recv_cnt = read(sock, (char*)&file_len, INT_SIZE);
		if (recv_cnt == -1)
			error_handling("read() file_len error!");
		recv_len += recv_cnt;
	}

	file_name[name_len] = 0;
	file = fopen(&file_name[1], "wb");

	recv_len = 0;
	while (recv_len < file_len) {
		recv_cnt = read(sock, &file_buf[recv_len], 1);
		if (recv_cnt == -1)
			error_handling("read() file error!");

		if (fwrite(&file_buf[recv_len], sizeof(char), recv_cnt, file) == 0)
			error_handling("fwrite() error!");
		
		recv_len += recv_cnt;
	}

	if (file == NULL)
		error_handling("fopen()  error!");


	shutdown(sock, SHUT_RDWR);
	fclose(file);

	return 0;
}

void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

