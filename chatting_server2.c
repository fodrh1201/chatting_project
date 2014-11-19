#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define BUF_SIZE 1024
#define EPOLL_SIZE 50
#define NAME_SIZE 1024
#define MAX_NUM 100	


void error_handling(char* message);
int write_to_all(int* clnt_list, int u_len, char* user_name, int t_len, char* message);

int main(int argc, char* argv[]) {
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t adr_sz;
	char user_name[NAME_SIZE];
	char message[BUF_SIZE];
	int clnt_list[MAX_NUM];
//	char* name_list[MAX_NUM];

	int i;
	int u_len;
	int t_len;
	int read_cnt, read_len, write_cnt, write_len;

	struct epoll_event* ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	memset(clnt_list, 0, MAX_NUM);

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error!");

	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	epfd = epoll_create(EPOLL_SIZE);
	ep_events = (struct epoll_event*)malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

	while (1) {
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);

		if (event_cnt == -1)
			error_handling("epoll_wait() error!");

		for (i = 0; i < event_cnt; i++) {
			if (ep_events[i].data.fd == serv_sock) {
				adr_sz = sizeof(clnt_adr);
				clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
				clnt_list[clnt_sock] = 1;
				event.events = EPOLLIN;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				printf("connected client : %d\n", clnt_sock);
			}
			else {
				printf("sork anjf wkfahtgoTsi");
				int fd = ep_events[i].data.fd;
				read(fd, (char*)&u_len, 1);
				
				read_len = 0;
				read_cnt = 0;
				while (read_len < u_len) {
					read_cnt = read(fd, user_name + read_len, u_len - read_len);
					if (read_cnt == -1)
						error_handling("read name error!");
					read_len += read_cnt;
				}

				if (u_len != read_len)
					error_handling("total name read error!");

				read(fd, (char*)&t_len, 1);

				read_len = 0;
				read_cnt = 0;
				while (read_len < t_len) {
					read_cnt = read(fd, message + read_len, t_len - read_len);
					if (read_cnt == -1)
						error_handling("read text error!");
					read_len += read_cnt;
				}

				if (!strcmp(message, "exit\n")) {
					epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
					clnt_list[ep_events[i].data.fd] = 0;
					close(ep_events[i].data.fd);
					printf("closed client : %d\n", ep_events[i].data.fd);
				}
				else {
					printf("%s > %s \n", user_name, message);
					write_len = write_to_all(clnt_list, u_len, user_name, t_len, message);
				}
				
				if (write_len != u_len + t_len + 2)
					error_handling("total write() error!");
			}
		}
	}
	close(serv_sock);
	close(epfd);
	free(ep_events);
	return 0;
}



int write_to_all(int* clnt_list, int u_len, char* user_name, int t_len, char* message) {
	int i;
	int total_len;
	int write_len, write_cnt;

	for (i = 0; i < MAX_NUM; i++) {
		total_len = 0;
		if (clnt_list[i] == 1) {
			int temp = (char)u_len;
			total_len += write(i, (char*)&temp, 1);

			write_len = 0;
			while (write_len < u_len) {
				write_cnt = write(i, user_name + write_len, NAME_SIZE);
				if (write_cnt == -1)
					error_handling("write() error!");
				write_len += write_cnt;
			}

			if (write_len != u_len)
				error_handling("write() name error!");

			total_len += write_len;

			temp = (char)t_len;
			total_len += write(i, (char*)&temp, 1);

			write_len = 0;
			while (write_len <t_len) {
				write_cnt = write(i, message + write_len, BUF_SIZE);
				if (write_cnt == -1)
					error_handling("write() error2!");
				write_len += write_cnt;
			}

			if (write_len != t_len)
				error_handling("write() message error!");
			total_len += write_len;
		}
	}
	return total_len;

}

void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


