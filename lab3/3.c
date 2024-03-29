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
fd_set fds;
int sfd, fd;

int clients_initialize() {
	// set zero
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	for (int i = 0;i < MAX_USERS;i++) {
		if (!used[i])
			continue;
		else {
			sfd = (sfd >= client[i]) ? sfd: client[i];
			FD_SET(client[i], &fds);
		}
	}
	return 1;
}

int add_client(int new_client) {
	for (int i = 0; i < MAX_USERS; i++) {
		if (used[i])
			continue; 
		else {
			used[i] = 1;
			client[i] = new_client;
			return 1;
		}
	}
	return 0;
}

void handle_chat(int id) {
	char buffer[BUF_LENGTH];
	ssize_t len;
	int symb, first = 1;
	int i, j, sig;
	int num = 0;
	int sended_length = 0;
	int left = 0;
	while (1) {
		// recv's return value: the length of content received	
		len = recv(client[id], buffer, BUF_LENGTH - 12, 0);
		if (len <= 0 && first) {
			used[id] = 0;
			close(client[id]);
			return;
		}
		else if(len <= 0) 
			return;
		first = 0;

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
				memset(msg, 0, sizeof(msg));
				sig = i + 1;
				num = 0;
			}
			else {
				msg[num] = buffer[i];
				num++;
			}
		}

		// detect if there's message remaining
		if(sig == len) 
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
				memset(msg, 0, sizeof(msg));
				num = 0;
			}
			else { } // do nothing
		}
		else { } // do nothing
	}
	return;
}

int handle_msg() {
	for (int i = 0;i < MAX_USERS;i++)
		if (used[i])
			if (FD_ISSET(client[i], &fds))
				handle_chat(i);
}

int main(int argc, char** argv) {
	int port = atoi(argv[1]);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket");
		return 1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	socklen_t addr_len = sizeof(addr);

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("bind");
		return 1;
	}

	if (listen(fd, 32)) {
		perror("listen");
		return 1;
	}

	int i, flag, ret;
	sfd = fd;
	int new_client;

	memset(used, 0, sizeof(used));
	memset(client, 0, sizeof(client));

	while (1) {
		ret = clients_initialize();
		if(ret) {
			// check if it's readable
			flag = select(sfd + 1, &fds, NULL, NULL, NULL);
			if (flag) {
				ret = FD_ISSET(fd, &fds);
				if (ret) {
					new_client = accept(fd, NULL, NULL);
					// set to non_block
					fcntl(new_client, F_SETFL, fcntl(new_client, F_GETFL, 0) | O_NONBLOCK);
					// new client
					int rt = add_client(new_client);
					if(!rt)
						printf("failed to add a new client!");
				}
				else handle_msg();
			}
		}
	}

	return 0;
}
