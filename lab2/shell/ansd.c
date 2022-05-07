#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_INPUT_LENGTH 256 // shell input
#define MAX_INPUT_ARGS 128
#define MAX_OUTPUT_LENGTH 2048

char Username[MAX_INPUT_LENGTH];
char Hostname[MAX_INPUT_LENGTH];
char CurWorkPath[MAX_OUTPUT_LENGTH];
char RootWorkPath[MAX_OUTPUT_LENGTH];
char cmd[MAX_INPUT_LENGTH];
char RootPath[MAX_OUTPUT_LENGTH];
int flag;
pid_t pidm;

int chartoint(char* buffer);
int execute_single_command(char **args);
void getUsername();
void getHostname();
int getCurWorkPath();
int getRootWorkPath();
int execute_with_redirection(char **args);
int execute_top_function(char **args);
void my_func(int sig);
void my_func0(int sig);


//main function
int main() {
	pid_t pid, pidt;
	int i, j, k, m, num, status;
	int ch;
	char temp[MAX_INPUT_LENGTH];
	char env[MAX_INPUT_LENGTH];
	char *args[MAX_INPUT_ARGS]; 	// define a string* array - each block represent a word (after seperation)
	char cmd[MAX_INPUT_LENGTH];
	FILE *fp = NULL;
	
	// get the path to save the history file
	int res = getRootWorkPath();
	if (!res) {
		printf("Error in get RootWorkPath\n");
		return -1;
	}
	pidm = getpid();
	strcpy(RootPath, RootWorkPath);
	strcat(RootPath, "/history.txt");
	//printf("%s", RootPath);

	// store the history 
	fp = fopen(RootPath, "rb+");
	rewind(fp);
	fread(&ch, sizeof(int), 1, fp);
	// printf("%d\n", ch);
	
	// self defined targed code
	if(ch != 6) {
		num = 0;
		ch = 6;
		fseek(fp, -1L, 1);
		fwrite(&ch, sizeof(int), 1, fp);
		printf("first time history info written!\n");
		m = 0;
	}
	else {
		for(m = 0; m <= 100; m++) {
			fread(&num, sizeof(int), 1, fp);
			// printf("%d ", num);
			if(num == 0) {
				fseek(fp, -sizeof(int), 1);
				break;
			}
			fread(temp, MAX_INPUT_LENGTH*sizeof(char), 1, fp);
			// printf("%s\n", temp);
		}
	}


	/* prepare for the bash info output */
	res = getCurWorkPath();
	if (!res) {
		printf("Error in get CurWorkPath\n");
		return -1;
	}
	getUsername();
	getHostname();
	
	// execute part
	while (1) {
		flag = 0;
		// leading output info
		printf("\e[32;1m%s@%s:\e[0m~", Username, Hostname);
		printf("\e[31;1m%s\e[0m $ ", CurWorkPath);

		signal(SIGINT, my_func0);

		// clear the buffer
		fflush(stdin);

		// cmd store the input command
		fgets(cmd, MAX_INPUT_LENGTH, stdin);

		flag = 1;	
		
		for (int i = 0; ; i++) {
			if(cmd[i] == '\n') {
				cmd[i] = '\0';
				strncpy(temp, cmd, MAX_INPUT_LENGTH*sizeof(char));
				break;
			}
			else if(cmd[i] == EOF) {
				num = 0;
				fwrite(&num, sizeof(int), 1, fp);
				fclose(fp);
				printf("\n");
				exit(1);
			}
		}	

		// analyse the command
		int i;

		// execute the !n or !! command
		here:
		args[0] = cmd;
		for (i = 0; *args[i]; i++)
			for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
	 			if (*args[i+1] == ' ') {
					*args[i+1] = '\0';
					args[i+1]++;
					break;
				}

		if(i == 0) 
			continue;
		else
			args[i] = NULL;
		 
		k = 0;
		for(j = 0; j < i; j++){
			if(*(args[j]) == '|') {
				k = 1;
			}
		}
		

		// a series of history command
		int number;
		if (strcmp(args[0], "history") == 0) {
			FILE* rd = fopen(RootPath, "r");
			fread(&number, sizeof(int), 1, rd);
			int low = chartoint(args[1]);
			for(int x = 0; x <= m-1; x++) {
				fread(&number, sizeof(int), 1, rd);
				fread(temp, MAX_INPUT_LENGTH*sizeof(char), 1, rd);
				if(x >= (m-low))
					printf("%d %s\n", number, temp);
			}
			fclose(rd);
			continue;
        	}

		if (args[0][0] == '!') {
			if(args[0][1] == '!') {
				FILE* rd = fopen(RootPath, "r");
				fread(&number, sizeof(int), 1, rd);
				for(int x = 0; x <= m-1; x++) {
					fread(&number, sizeof(int), 1, rd);
					fread(temp, MAX_INPUT_LENGTH*sizeof(char), 1, rd);
					if(x == m-1) {
						printf("%s\n", temp);
						fclose(rd);
						strcpy(cmd, temp);
						goto here;
					}
				}
			}
			else {
				int nm;
				strncpy(temp, args[0]+1, (MAX_INPUT_LENGTH-1)*sizeof(char));
				temp[MAX_INPUT_LENGTH-1] = '\0';
				nm = chartoint(temp);
				FILE* rd = fopen(RootPath, "r");
				fread(&number, sizeof(int), 1, rd);
				for(int x = 0; x <= m-1; x++) {
					fread(&number, sizeof(int), 1, rd);
					fread(temp, MAX_INPUT_LENGTH*sizeof(char), 1, rd);
					if(x == nm-1) {
						printf("%s\n", temp);
						fclose(rd);
						strcpy(cmd, temp);
						goto here;
					}
				}
        		}
		}
	

		// store the history
		// the history command itself won't be saved
		pidt = getpid();
		if(pidt == pidm) {
			m++;
			fwrite(&m, sizeof(int), 1, fp);
			fwrite(temp, MAX_INPUT_LENGTH*sizeof(char), 1, fp);
		}
		fclose(fp);
		fp = fopen(RootPath, "rb+");
		fseek(fp, (sizeof(int)+m*(sizeof(int)+MAX_INPUT_LENGTH*sizeof(char))), 0);	


		if(k == 0) {
			execute_top_function(args);
		}
		else {
			// In order to deal with Ctrl+C, Create a son process here, exit when recieve Ctrl+C
			pid = fork();
			if (pid == 0){
				/* Learning Note:

				to detect the Ctrl + C

				function: signal()

				void (*signal(int sig, void (*func)(int)))(int);

				SIGINT:	(Signal Interrupt) interrupt signal like "ctrl+C", usually genererated by the user
				
				*/		
				signal(SIGINT, my_func);
								
				// This part is used to deal with environment variables
				for (i = 0; args[i]; i++) {
					for (k = 0; args[i][k]; k++){
						if (args[i][k] == '$') {
							for (j = k+1; ; j++)
								// detect the legal character string: '_', '1'~'9'
	 							// and also: 'a'~'z', 'A'~'Z'
								if(((args[i][j] >= 'a') && (args[i][j] <= 'z')) || ((args[i][j] > 'A') && (args[i][j] < 'Z'))) {
									temp[j - (1+k)] = args[i][j];
									continue;
								}
								else if (((args[i][j] > '1') && (args[i][j] < '9')) || (args[i][j] == 95)) {
									temp[j - (1+k)] = args[i][j];
									continue;
								}
								else break;
							temp[j - (1+k)] = '\0';
							if (getenv(temp) == NULL) {
								printf("wrong input env name\n");
								goto end_of_loop;
							}
							else {
								strcpy(env, getenv(temp));
								if (args[i][j])
									strcat(env, args[i]+j);
							 	strcpy(args[i]+k, env);
							}
							temp[j - (1+k)] = args[i][j];
						}
					}                
				}
				execute_top_function(args);
				end_of_loop: 
				num = 0;
				fwrite(&num, sizeof(int), 1, fp);
				fclose(fp);
				exit(0);
			}
			else{
				// detect if this is the top process (shell)
				// Ignore the Ctrl + C in parent process to avoid exiting the program	
				signal(SIGINT,SIG_IGN);
				waitpid(pid, &status, 0);
				fseek(fp, (sizeof(int)+m*(sizeof(int)+MAX_INPUT_LENGTH*sizeof(char))), 0);		
				if (WEXITSTATUS(status) == 1) 
					return 0; // exit the program.
			}
		}
	}
}

int chartoint(char* buffer) {
	int i, j, sum = 0;	
	for (i = 0; buffer[i]; i++) ;
	for (j = 0; j < i ; j++) {
		// printf("%c\n", buffer[j]);
		if(buffer[j] >= '0' && buffer[j] <='9') {
			sum=sum*10+buffer[j]-'0';
			// printf("%d",buffer[j]-'0');
		}
		// i represents the '\0'
		else if( j != i )
			return -10;
	}	
	return sum;
}

int execute_single_command(char **args){
	int i, res;

	// input nothing
	if (!args[0])
        	return 0;

	// inner command
	// pwd
        if (strcmp(args[0], "pwd") == 0) {
		char path[MAX_OUTPUT_LENGTH];
		char* ot = getcwd(path, MAX_OUTPUT_LENGTH);
		if(ot == NULL) {
			printf("shell: pwd failed\n");
			printf("Current path is too long to display!\n");
		}
		else puts(ot);
		return 0;
        }
	
	// cd command
	if (strcmp(args[0], "cd") == 0) {
		if (args[1]) {
    			res = chdir(args[1]);
			if(res == -1)
				printf("shell: No such file or directory\n");
		}
		res = getCurWorkPath();
		if (!res) {
			printf("Error in get CurWorkPath\n");
			return -1;
		}
		return 0;
	}

	// export
	if (strcmp(args[0], "export") == 0) {
		for (i = 1; args[i] != NULL; i++) {
			/*处理每个变量*/
			char *name = args[i];
			char *value = args[i]+ 1;
			while (*value != '\0' && *value != '=')
			value++;
			*value = '\0';
			value++;
			setenv(name, value, 1);
		}
		return 0;
	}

	// exit
	if (strcmp(args[0], "exit") == 0) {
		exit(1);
	}

	// extermal command
	pid_t pid2 = fork();
	if (pid2 == 0) {
		// Been added here. In order to kill all son process when the Ctrl+C is detected.
		prctl(PR_SET_PDEATHSIG,SIGKILL);
		// child process
		execvp(args[0], args);
		// faile to execvp
		exit(255);
	}

        // only parent process could access
	waitpid(pid2,NULL,0);
	return 0;
}

void getUsername() {
	struct passwd* pwd = getpwuid(getuid());
	strcpy(Username, pwd->pw_name);
}

void getHostname() { 
	gethostname(Hostname, MAX_INPUT_LENGTH);
}

int getCurWorkPath() { 
	char* result = getcwd(CurWorkPath, MAX_OUTPUT_LENGTH);
	if (result == NULL) {
		printf("shell: ERROR_SYSTEM\n");
		return 0;
	}
	else return 1;
}

int getRootWorkPath() { 
	char* result = getcwd(RootWorkPath, MAX_OUTPUT_LENGTH);
	if (result == NULL) {
		printf("shell: ERROR_SYSTEM\n");
		return 0;
	}
	else return 1;
}

// execute_top_function
// detect pipe and create pipe for the args to execute
// creates a child process and executes the part before the '|' (normally with only one command)
int execute_top_function(char **args) {
	int i,status;
	pid_t pid;
	int fd[2],sfd;
	// save the STDIN so that ew can reload it after executed the whole instruction
	// actually, dup(STDIN_FILENO) is the same as fcntl(STDIN_FILENO, F_DUPFD, 0)
	// we have to make sure that all the FILENO is restored to present the unexpected bugs
	// the bug is caused because we change the ptr field of STD*_FILENO (an analogy) 
	sfd = dup(STDIN_FILENO); 
	for (i = 0; args[i] && *args[i]!='|'; i++) ;
	if (args[i]) {
		// change the '|' to NULL
		args[i] = NULL;
		/*
		int pipe(int fd[2]);
		success return 0
		failed return -1
		*/
		int ret = pipe(fd);
		if(ret == -1) {
			printf("pipe: create pipe failed\n");
		}
		// examine if there's any pipe structure.
		// pid != 0, parent process
		pid = fork();
		if (pid != 0) {
			waitpid(pid, NULL, 0);
			dup2(fd[0], STDIN_FILENO); // STDIN_FILENO is covered by fd[0]
			close(fd[1]);
			close(fd[0]);
			// rescusion
			execute_top_function(&args[i+1]);
			// restore the STDIN_FILENO
			dup2(sfd,STDIN_FILENO);
			return 0;
		}
		// child process
		else {
			close(fd[0]);
			dup2(fd[1],STDOUT_FILENO);
			close(fd[1]);
			execute_with_redirection(args); 
			exit(0);
		}	
	}
	else return execute_with_redirection(args);
}


// execute_with_redirection function
// detect and deal with the symbol character: '>', '>>', '<'
// try to achieve the connection to server
// can handle command like: /dev/stdin, /dev/stdout, /dev/stderr etc
int execute_with_redirection(char **args){
	int i, j;
	// to cover the file or not
	int Cover;
	int port_in, port_out, s_in, s_out;
	int counter[3] = {0, 0, 0};
	int tcp_flag[2] = {0,0};	
	char *InFile, *OutFile;
	char cmp_string[10];
	char IP_in[MAX_OUTPUT_LENGTH], IP_out[MAX_OUTPUT_LENGTH];
	pid_t pid;
	FILE *fp;
	/*
	struct sockaddr_in {
	     short            sin_family;    // 2 byte , addr_family，e.g. AF_INET, AF_INET6
	     unsigned short   sin_port;      // 2 byte , 16-bit TCP/UDP port number e.g. htons(3490)，
	     struct in_addr   sin_addr;      // 4 byte , 32-bit IP address
	     char             sin_zero[8];   // 8 byte 
	};
	
	defined in the head file: #include <netinet/in.h> or #include <arpa/inet.h>
	*/
	struct sockaddr_in addr_in, addr_out;
	for (i = 0; args[i]; i++){
		// detect the symbol character: '>', '>>', '<'
		if (strcmp(args[i], ">") == 0){
			args[i] = NULL;
			//detect if there's "/dev/tcp/"
			i++;
			strncpy(cmp_string,args[i],9);
			if (strcmp(cmp_string, "/dev/tcp/") == 0){
				// get the IP address and the Port number
				for(j = 9; args[i][j]!='/';IP_out[j-10] = args[i][j++]);
				IP_out[j-9] = '\0';
				port_out = atoi(args[i]+j+1);
				tcp_flag[1] = 1;
			}
			else OutFile = args[i];
			// cover the file
			Cover = 1;
			counter[0]++;
		}
		if (strcmp(args[i], ">>") == 0){
			args[i] = NULL;
			OutFile = args[++i];
			// not cover the file
			Cover = 0;
			counter[1]++;
		}
		if (strcmp(args[i], "<") == 0){
			args[i] = NULL;
			strncpy(cmp_string,args[++i],9);
			//detect if there's "/dev/tcp/"
			if (strcmp(cmp_string, "/dev/tcp/") == 0){
				tcp_flag[0] = 1;
				for(j = 9; args[i][j]!='/'; IP_in[j-10] = args[i][j++]);
				IP_in[j-9] = '\0';
				port_in = atoi(args[i]+j+1);
			}
			else InFile = args[i];
			counter[2]++;
		}
	}
	if ((counter[0] || counter[1] || counter[2]) == 0)
		return execute_single_command(args);
	if (counter[2] && !tcp_flag[0]){
		if ((fp = fopen(InFile,"r")) == NULL) {
			printf("TCP part: Input File Error\n");
			exit(0);
		}
		else fclose(fp);
	}
	if (counter[2] > 1 || (counter[0] + counter[1]) > 1){
		printf("ERROR: Too much redirection.\n");
		return 0;
	}
	//create a child process 
	pid = fork();
	if (pid == 0){
		if (counter[2]){
			if (tcp_flag[0]){
				//use socket() and connect() to connect to the server
				if((s_in = socket(AF_INET,SOCK_STREAM,0))<0){
					printf("socket error\n");
					exit(1);
				}
				addr_in.sin_family = AF_INET;
				addr_in.sin_port=htons(port_in);
				addr_in.sin_addr.s_addr = inet_addr(IP_in);
				if (-1 == connect(s_in, (struct sockaddr*)(&addr_in), sizeof(struct sockaddr))){
					printf("connect error\n");
					exit(1);
				}
				dup2(s_in, STDIN_FILENO);
			}
			else freopen(InFile, "r", stdin);
		}
		if (tcp_flag[1])
			printf("Out Part:\nIP:%s,port:%d\n", IP_out, port_out);
		if (tcp_flag[0])
			printf("In Part:\nIP:%s,port:%d\n", IP_in,  port_in );
		if ((counter[0] + counter[1]) && !Cover)
			freopen(OutFile, "a", stdout);
		if ((counter[0] + counter[1]) && Cover){
			if (tcp_flag[1]){
				//use socket() and connect() to connect to the server
				if((s_out = socket(AF_INET,SOCK_STREAM,0))<0){
					printf("socket error\n");
					exit(1);
				}
				addr_out.sin_family = AF_INET;
				addr_out.sin_port = htons(port_out);
				addr_out.sin_addr.s_addr = inet_addr(IP_out);
				if (-1 == connect(s_out, (struct sockaddr*)(&addr_out), sizeof(struct sockaddr))){
					printf("connect error\n");
					exit(1);
				}
				dup2(s_out, STDOUT_FILENO);
			}
			else
				freopen(OutFile, "w", stdout);
		} 
		execute_single_command(args);
		if (tcp_flag[0]) 
			close(s_in);
		if (tcp_flag[1])
			close(s_out);
		exit(0);
	}
	else {
		waitpid(pid,NULL,0);
		return 0;
	}
}


// Used in main(), the signal() func_function
void my_func(int sig){
    if (sig == SIGINT){
	fflush(stdout);
        printf("\n");
        exit(0);
    }
}

// Used in main(), the signal() func_function
void my_func0(int sig){
	// clear the buffer
	if(!flag) {
		printf("\n");
		printf("\e[32;1m%s@%s:\e[0m~\e[31;1m%s\e[0m $ ", Username, Hostname, CurWorkPath);
		fflush(stdout);		
	}
	else {	
		printf("\n");
		fflush(stdin);
	}
}


