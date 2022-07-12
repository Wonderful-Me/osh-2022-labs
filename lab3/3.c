#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>

#define BUF_LENGTH 1048600
#define MAX_MSG 1048600
#define MAX_USERS 32

int used[MAX_USERS];
int client[MAX_USERS];

char msg[MAX_MSG] = "";

void handle_chat(int id) {
	char buffer[BUF_LENGTH];
	ssize_t len;
	int symb, first = 1;
	int i, j;
	int num = 0;
	int sended_length = 0;
	int left = 0;
	while (1) {
		// recv's return value: the length of content received	
		len = recv(client[id], buffer, BUF_LENGTH - 12, 0);
		if (len <= 0) {
			if (first) {
				used[id] = 0;
				close(client[id]);
				return;
			}
			return;
		}
		else first = 0;


		for (i = 0; i < len; i++) {
			if (buffer[i] == '\n') {
				msg[num] = buffer[i];
				num++;
				for (j = 0; j < MAX_USERS; j++) {
					left = num;
					sended_length = 0;
					if (used[j] && j != id) {
						while (sended_length < num) {
							sended_length += send(client[j], msg + sended_length, left, 0);
							left = num - sended_length;
						}
					}
				}
				num = 0;
			}
			else {
				msg[num] = buffer[i];
				num++;
			}
		}

		// detect if there's message remaining
		if(i == len) 
			symb = 0;
		else symb = 1;
		
		if (symb) {
			// last msg remaining
			if (len < BUF_LENGTH - 12) {
				for (j = 0; j < MAX_USERS; j++) {
					left = num;
					sended_length = 0;
					if (used[j] && j != id) {
						while (sended_length < num) {
							sended_length += send(client[j], msg + sended_length, left, 0);
							left = num - sended_length;
						}
					}
				}
				num = 0;
			}
			else { } // do nothing
		}
		else { } // do nothing
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
		for (i = 0;i < MAX_USERS;i++)
			if (used[i])
				FD_SET(client[i], &fds);
		
		// check if it's readable
		//  maxfdp: sfd + 1 —— 传入参数，集合中所有文件描述符的范围，即最大文件描述符值+1
		if (select(sfd + 1, &fds, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(fd, &fds)) {
				int new_client = accept(fd, NULL, NULL);
				if (new_client == -1) {
					perror("accept");
					return 1;
				}

				// set to non_block
				fcntl(new_client, F_SETFL, fcntl(new_client, F_GETFL, 0) | O_NONBLOCK);

				if (sfd < new_client) {
					sfd = new_client;
				}

				// new client
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
