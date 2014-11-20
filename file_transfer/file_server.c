#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define	BUF_SIZE	1024
#define	INT_SIZE		4
#define FSIZE		167776

void error_handling(char* message);

int main(int argc, char* argv[]) {
	int serv_sock;
	int clnt_sock;
	int recv_len;

	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size;
	char file_Buffer[FSIZE];
	int file_len, file_cnt;
	int file_name_len, file_name_cnt;
	int write_len, write_cnt;
	int read_cnt;
	char file_name[BUF_SIZE];
	struct stat st;

	FILE* file;

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1) 
		error_handling("socket() error!");

	memset(&serv_addr, 0,sizeof(serv_addr));;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error!");

	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error!");

	clnt_addr_size = sizeof(clnt_sock);
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
	
	if (clnt_sock == -1)
		error_handling("accept() error!");

	if (read(clnt_sock, (char*)&file_name_len, 1) == -1)
		error_handling("read() file_name_len error!");

	recv_len = 0;
	file_name_cnt = 0;
	while (recv_len < file_name_len) {
		file_name_cnt = read(clnt_sock, &file_name[recv_len+1], 1);
		if (file_name_cnt == -1)
			error_handling("read() file_name error!");
		recv_len += file_name_cnt;
	}

	file_name[recv_len]=0;
	file = fopen(&file_name[1], "rb");
	if (file == NULL) {
		close(clnt_sock);
		close(serv_sock);
		error_handling("no file");
	}

	if (lstat(&file_name[1], &st) == -1) {
		close(clnt_sock);
		close(serv_sock);
		error_handling("path error!");
	}	

	file_len = (int)st.st_size;
	printf("%d\n", file_len);
	write(clnt_sock, (char*)&file_len, INT_SIZE);

	int write_acc = 0;
	write_len = 0;
	read_cnt = 0;
	while ((read_cnt = fread(file_Buffer, sizeof(char), FSIZE, file)) != 0) {
		write_acc = 0;
		while (read_cnt) {
			write_cnt = write(clnt_sock, file_Buffer + write_len, read_cnt);
			if (write_cnt < 0)
				error_handling("write() error!");
			write_acc += write_cnt;
			read_cnt -= write_cnt;
		}
		write_len += write_acc;
	}

	if (file_len != write_len)
		error_handling("write() total length error!");

	printf("Sending Success\n");
	shutdown(clnt_sock, SHUT_RDWR);
	shutdown(serv_sock, SHUT_RDWR);
	fclose(file);

	return 0;
}

void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}





























