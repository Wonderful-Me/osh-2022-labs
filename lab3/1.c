#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_LENGTH 1048576

struct Pipe {
	int fd_send;
	int fd_recv;
};

void* handle_chat(void* data) {
	struct Pipe* pipe = (struct Pipe*)data;
	char msg[BUF_LENGTH];
	char buffer[BUF_LENGTH];
	memset(msg, 0, sizeof(msg));
	memset(buffer, 0, sizeof(buffer));
	ssize_t len = 0;
	int i, tag, num, left, sended_length, symb;
	symb = 0;	
	tag = 0; 
	num = 0;
	left = 0;
	while (1) {
		// recv's return value: the length of content received
		len = recv(pipe->fd_send, buffer, BUF_LENGTH - 12, 0);
		for (i = 0; i < len; i++) {
			if (buffer[i] == '\n') {
				msg[num] = buffer[i];
				num++;
				left = num;
				sended_length = 0;
				while (sended_length < num) {
					sended_length += send(pipe->fd_recv, msg + sended_length, left, 0);
					left = num - sended_length;
				}
				tag = i + 1;
				num = 0;
			}
			else {
				msg[num] = buffer[i];
				num++;
			}
		}

		// detect if there's message remaining
		if(tag == len) 
			symb = 0;
		else symb = 1;

		if (symb) {
			// last msg remaining
			if (len < BUF_LENGTH - 12) {
				left = num;
				sended_length = 0;
				while (sended_length < num) {
					sended_length += send(pipe->fd_recv, msg + sended_length, left, 0);
					left = num - sended_length;
				}
				tag = 0;
				num = 0;
			}
			else { } // do nothing
		}
		else { } // do nothing
	}
	return NULL;
}

int main(int argc, char** argv) {
	int port = atoi(argv[1]);
	int fd;
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

	if (listen(fd, 2)) {
		perror("listen");
		return 1;
	}

	int fd1 = accept(fd, NULL, NULL);
	int fd2 = accept(fd, NULL, NULL);
	if (fd1 == -1 || fd2 == -1) {
		perror("accept");
		return 1;
	}

	pthread_t thread1, thread2;
	struct Pipe pipe1;
	struct Pipe pipe2;
	pipe1.fd_send = fd1;
	pipe1.fd_recv = fd2;
	pipe2.fd_send = fd2;
	pipe2.fd_recv = fd1;
	pthread_create(&thread1, NULL, handle_chat, (void*)&pipe1);
	pthread_create(&thread2, NULL, handle_chat, (void*)&pipe2);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);

	return 0;
}
