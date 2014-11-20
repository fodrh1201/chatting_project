#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100


void* request_handler(void* arg);
void send_data(FILE* fp, char* ct, char* file_name);
char* content_type(char* file);
void send_error(FILE* fp);
void error_handling(char* message);

int main(int argc, char* argv[]) {
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_size;
	char buf[BUF_SIZE];
	pthread_t t_id;
	if (argc != 2) {
		printf("Usage : %s <port> \n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error!");

	if (listen(serv_sock, 20) == -1)
		error_handling("listen() error");

	while (1) {
		clnt_adr_size = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
		printf("Connection Request : %s:%d\n",
				inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
		pthread_create(&t_id, NULL, request_handler, &clnt_sock);
		pthread_detach(t_id);
	}
	close(serv_sock);
	return 0;
}

void* request_handler(void *arg) {
	int clnt_sock = *((int*)arg);
	char req_line[SMALL_BUF];
	FILE* clnt_read;
	FILE* clnt_write;

	char method[10];
	char ct[15];
	char file_name[30];

	clnt_read = fdopen(clnt_sock, "rb");
	clnt_write = fdopen(dup(clnt_sock), "wb");
	fgets(req_line, SMALL_BUF, clnt_read);
	printf("fget req_line success\n");

	if (strstr(req_line, "HTTP/") == NULL) {
		send_error(clnt_write);
		fclose(clnt_read);
		fclose(clnt_write);
		pthread_exit((void*)NULL);
	}

	strcpy(method, strtok(req_line, " /"));
	printf("method : %s\n", method);
	strcpy(file_name, strtok(NULL, " /"));
	printf("file_name : %s\n", file_name);
	strcpy(ct, content_type(file_name));
	printf("ct : %s\n", ct);
	if (strcmp(method, "GET") != 0) {
		send_error(clnt_write);
		fclose(clnt_read);
		fclose(clnt_write);
		pthread_exit((void*)NULL);
	}

	fclose(clnt_read);
	printf("clnt_sock : %d\n", clnt_sock);
	send_data(clnt_write, ct, file_name);
	pthread_exit((void*)NULL);
}

void send_data(FILE* fp, char* ct, char* file_name) {
	char protocol[] = "HTTP/1.0 200 OK\r\n";
	char server[] = "Server:Linux Web Server \r\n";
	char cnt_len[] = "Content-length:2048000\r\n";
	char cnt_type[SMALL_BUF];
	char buf[BUF_SIZE];
	int read_cnt, read_len, file_len, write_len, write_cnt, str_len;
	FILE* send_file;
	struct stat st;

	sprintf(cnt_type, "Content-type:%s\r\n\r\n", ct);
	printf("file_name : %s\n", file_name);
	send_file = fopen(file_name, "rb");
	if (send_file == NULL) {
		send_error(fp);
		return;
	}

	str_len = strlen(protocol);
	write_len = 0;
	while (write_len < str_len) {
		write_cnt = fwrite(protocol, 1, str_len - write_len, fp);
		if (write_cnt == -1)
			error_handling("write() error 1");
		write_len += write_cnt;
	}

	str_len = strlen(server);
	write_len = 0;
	while (write_len < str_len) {
		write_cnt = fwrite(server, 1, str_len - write_len, fp);
		if (write_cnt == -1)
			error_handling("write() error 1");
		write_len += write_cnt;
	}

	str_len = strlen(cnt_len);
	write_len = 0;
	while (write_len < str_len) {
		write_cnt = fwrite(cnt_len, 1, str_len - write_len, fp);
		if (write_cnt == -1)
			error_handling("write() error 1");
		write_len += write_cnt;
	}

	str_len = strlen(cnt_type);
	write_len = 0;
	while (write_len < str_len) {
		write_cnt = fwrite(cnt_type, 1, str_len - write_len, fp);
		if (write_cnt == -1)
			error_handling("write() error 1");
		write_len += write_cnt;
	}
	printf("header success!\n");


	if (lstat(file_name, &st) == -1)
		error_handling("lstat error!");

	file_len = (int)st.st_size;
	printf("file_len : %d\n", file_len);

	read_len = 0;
	while (read_len < file_len) {
		read_cnt = fread(buf, 1, BUF_SIZE, send_file);
		printf("read_cnt : %d\n", read_cnt);
		if (read_cnt == -1)
			error_handling("fread() error!");
		read_len += read_cnt;
		write_len = 0;
		while (write_len < read_cnt) {
			write_cnt = fwrite(buf + write_len, 1, read_cnt - write_len, fp);
			printf("write_cnt : %d\n", write_cnt);
			if (write_cnt == -1)
				error_handling("fwrite() error!");
			write_len += write_cnt;
		}
	}

	if (read_len != file_len)
		error_handling("file move error!");

	printf("request data success!\n");
	fclose(fp);
}

char* content_type(char* file) {
	char extension[SMALL_BUF];
	char file_name[SMALL_BUF];
	strcpy(file_name, file);
	strtok(file_name, ".");
	strcpy(extension, strtok(NULL, "."));

	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
		return "text/html";
	else
		return "text/plain";
}

void send_error(FILE* fp) {
	char protocol[] = "HTTP/1.0 400 Bad Request\r\n";
	char server[] = "Server:Linux Web Server \r\n";
	char cnt_len[] = "Content-length:2048\r\n";
	char cnt_type[] = "Content-type:text/html\r\n\r\n";
	char content[] = "<html><head><title>NETWORK</title></head>"
		"<body><font size=+5><br>오류 발생! 요청 파일명 및 요청 방식 확인!"
		"</font></body></html>";

	fputs(protocol, fp);
	fputs(server, fp);
	fputs(cnt_len, fp);
	fputs(cnt_type, fp);
	fputs(content, fp);
	fflush(fp);
}

void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
