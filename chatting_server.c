#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define	BUF_SIZE	1024
#define	EPOLL_SIZE	50

int write_to_all(int epfd, struct epoll_event* ep_events, int event_cnt, char* message, int size);
void error_handling(char* message);

int main(int argc, char* argv[]) {
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t adr_sz;
	int recv_len, recv_cnt, write_len, i;
	char buf[BUF_SIZE];

	struct epoll_event* ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	if (argc != 2) {
		printf("Usage: %s <port> \n", argv[0]);
		exit(1);
	}

	serv_sock =socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error!");

	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error!");

	epfd = epoll_create(EPOLL_SIZE);
	ep_events = malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

	while (1) {
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);

		if (event_cnt == -1) {
			puts("epoll_wait() error!");
			break;
		}

		for (i = 0; i < event_cnt; i++) {
			if (ep_events[i].data.fd == serv_sock) {
				adr_sz = sizeof(clnt_adr);
				clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
				event.events = EPOLLIN | EPOLLOUT;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				printf("connected client : %d \n", clnt_sock);
			}
			else {
				if ((ep_events[i].events & EPOLLIN) == EPOLLIN) {
					recv_len = 0;
					recv_cnt = 0;
					while ((recv_cnt = read(ep_events[i].data.fd, buf + recv_len, BUF_SIZE)) != 0) {
						if (recv_cnt == -1) 
							error_handling("read() error!");
	
						recv_len += recv_cnt;
					}
					if (recv_len == 0) {
						epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
						close(ep_events[i].data.fd);
						printf("closed client : %d \n", ep_events[i].data.fd);
					}
					else 
						write_len = write_to_all(epfd, ep_events, event_cnt, buf, recv_len);
				}
			}
		}
	}
	close(serv_sock);
	close(epfd);
	free(ep_events);
	return 0;
}

void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int write_to_all(int epfd, struct epoll_event* ep_events, int event_cnt, char* buf, int size) {
	int i;
	int write_len, write_cnt;	
	int read_len;
	char len;
	for (i = 0; i < event_cnt; i++) {
		if ((ep_events[i].events & EPOLLOUT) == EPOLLOUT) {
			read_len = size;
			len = (char) read_len;
			write(ep_events[i].data.fd, &len, 1);
			while (read_len) {
				write_cnt = write(ep_events[i].data.fd, buf + write_len, size);
				if (write_cnt == -1)
					error_handling("write() error!");
				write_len += write_cnt;
				read_len -= write_cnt;
			}

			if (size != write_len)
				error_handling("total write() error!");
		}
	}
}



