#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>

#define BUF 1048600
#define MAX_MSG 1048600
#define MAX_USERS 32

int used[MAX_USERS];
int client[MAX_USERS];

char msg[MAX_MSG] = "";

void handle_chat(int id) {
	char buffer[BUF];
	ssize_t len;
	int start = 8;
	int first_recv = 1;

	sprintf(msg, "user%2d: ", id);

	while (1) {
		len = recv(client[id], buffer, BUF - 12, 0);
		if (len <= 0) {
			if (first_recv) {
				used[id] = 0;
				close(client[id]);
				return;
			}
			return;
		}
		first_recv = 0;

		int i, j;
		int tag = 0;
		int num = 0;
		for (i = 0; i < len; i++) {
			if (buffer[i] == '\n') {
				num = i - tag + 1;
				strncpy(msg + start, buffer + tag, num);

				for (j = 0;j < MAX_USERS;j++) {
					if (used[j] && j != id) {
						int remain = start + num;
						int sended = 0;
						while (remain > 0) {
							sended = send(client[j], msg + sended, remain, 0);
							if (sended == -1) {
								perror("send");
								exit(-1);
							}
							remain -= sended;
						}
					}
				}
				tag = i + 1;
				start = 8;
			}
		}
		if (tag != len) {
			num = len - tag;
			strncpy(msg + start, buffer + tag, num);
			start = start + num;
		}
	}
	return;
}

int main(int argc, char** argv) {
	int port = atoi(argv[1]);
	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket");
		return 1;
	}

	memset(used, 0, sizeof(used));
	memset(client, 0, sizeof(client));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	socklen_t addr_len = sizeof(addr);

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("bind");
		return 1;
	}

	if (listen(fd, MAX_USERS)) {
		perror("listen");
		return 1;
	}

	int i;
	int sfd = fd;
	fd_set fds;
	while (1) {
		// set zero
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		for (i = 0;i < MAX_USERS;i++) {
			if (used[i])
				FD_SET(client[i], &fds);
		}
		
		// check if it's readable
		//  maxfdp: sfd + 1 ——传入参数，集合中所有文件描述符的范围，即最大文件描述符值+1
		if (select(sfd + 1, &fds, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(fd, &fds)) {
				int new_client = accept(fd, NULL, NULL);
				if (new_client == -1) {
					perror("accept");
					return 1;
				}

				fcntl(new_client, F_SETFL, fcntl(new_client, F_GETFL, 0) | O_NONBLOCK);

				if (sfd < new_client) {
					sfd = new_client;
				}

				for (i = 0; i < MAX_USERS; i++) {
					if (!used[i]) {
						used[i] = 1;
						client[i] = new_client;
						break;
					}
				}
			}
			else {
				for (i = 0;i < MAX_USERS;i++) {
					if (used[i] && FD_ISSET(client[i], &fds))
						handle_chat(i);
				}
			}
		}
	}

	return 0;
}
