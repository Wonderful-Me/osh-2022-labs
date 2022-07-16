#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF 2048
#define MAX_USERS 32
#define MAX_MESGS 56

struct Info {
	int fd_recv;
	int myid;
};

struct Send {
	int ori;
	int clt;
	int remain;
	char msg[1048600];
};

struct Send sd[MAX_USERS][MAX_MESGS];

int p[MAX_USERS], q[MAX_USERS];

int is_empty(int i) {
	if(p[i] == q[i])
		return 1;
	else return 0;
}

int is_full(int i) {
	if((q[i]+1) % MAX_MESGS == p[i])
		return 1;
	else return 0;
}

void Enqueue(int i, struct Send in) {
	if(!is_full(i)) {
		q[i] = (q[i] + 1) % MAX_MESGS;
		sd[i][q[i]] = in;
	}
}

struct Send Dequeue(int i) {
	if(!is_empty(i)) {
		p[i] = (p[i] + 1) % MAX_MESGS;
		return sd[i][p[i]];
	}
}

int client[MAX_USERS];

pthread_mutex_t mutex[MAX_USERS] = PTHREAD_MUTEX_INITIALIZER;
int used[MAX_USERS] = { 0 };

void* send_msg(void* x) {
	int *p = ( int*)x;
	int i = *p;
	char msg[1048600] = "";
	while (1) {
		if(used[i] && !is_empty(i)) {
			pthread_mutex_lock(&mutex[i]);
			struct Send in = Dequeue(i);
			pthread_mutex_unlock(&mutex[i]);
			int remain = in.remain;
			int client = in.clt;
			int num = in.remain;
			for(int k = 0; k < 1048600; k++)
				msg[k] = in.msg[k];
			int sended_length = 0;
			while (sended_length < num) {
				sended_length += send(client, msg + sended_length, remain, 0);
				remain = num - sended_length;
			}
		}
	}
	return NULL;
}

void* handle_chat(void* data) {
	struct Info* info = (struct Info*)data;
	struct Send inn;
	char buffer[BUF];
	ssize_t len;
	int i, j, symb;
	int tag = 0, num = 0;
	char msg[1048600] = "";
	while (1) {

		len = recv(info->fd_recv, buffer, BUF - 12, 0);

		if (len <= 0) {
			used[info->myid] = 0;
			close(client[info->myid]);
			return 0;
		}

		for (i = 0; i < len; i++) {
			if (buffer[i] == '\n') {
				msg[num] = buffer[i];
				num++;
				for (j = 0; j < MAX_USERS; j++) {
					if (used[j] && j != info->myid) {
						pthread_mutex_lock(&mutex[j]);
						inn.ori = info->myid;
						inn.clt = client[j];
						inn.remain = num;
						memset(inn.msg, 0, sizeof(inn.msg));
						for(int k = 0; k < 1048600; k++)
							inn.msg[k] = msg[k];
						Enqueue(j, inn);
						pthread_mutex_unlock(&mutex[j]);
					}
				}
				num = 0;
				tag = i + 1;
				memset(msg, 0, sizeof(msg));
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
			if (len < BUF - 12) {
				for (j = 0; j < MAX_USERS; j++) {
					if (used[j] && j != info->myid) {
						pthread_mutex_lock(&mutex[j]);
						inn.ori = info->myid;
						inn.clt = client[j];
						inn.remain = num;
						memset(inn.msg, 0, sizeof(inn.msg));
						for(int k = 0; k < 1048600; k++)
							inn.msg[k] = msg[k];
						Enqueue(j, inn);
						pthread_mutex_unlock(&mutex[j]);
					}
				}
				num = 0;
				memset(msg, 0, sizeof(msg));
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
	if (listen(fd, MAX_USERS)) {
		perror("listen");
		return 1;
	}

	pthread_t sdm[MAX_USERS];
	pthread_t thread[MAX_USERS];
	struct Info info[MAX_USERS];
	
	memset(sd, 0, sizeof(sd));
	memset(p, 0, MAX_USERS);
	memset(q, 0, MAX_USERS);
	while (1) {
		int i;
		int tmp_client = accept(fd, NULL, NULL);
		if (tmp_client == -1) {
			perror("accept");
			return 1;
		}
		for (i = 0; i < MAX_USERS; i++) {
			if (used[i] == 0) {
				used[i] = 1;
				client[i] = tmp_client;
				info[i].fd_recv = tmp_client;
				info[i].myid = i;
				pthread_create(&sdm[i], NULL, send_msg, (void*)&i);
				pthread_create(&thread[i], NULL, handle_chat, (void*)&info[i]);
				break;
			}
		}
	}
	return 0;
}
