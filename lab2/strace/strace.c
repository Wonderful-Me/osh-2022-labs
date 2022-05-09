#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <syscall.h>
#include <sys/ptrace.h>


int main(int argc, char **argv)
{
	if (argc <= 1) {
		printf("insufficient number of args\n");
		exit(1);
	}
	pid_t pid = fork();
	switch (pid) {
	case -1: // error
		exit(1);
	case 0:  // child
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		execvp(argv[1], argv + 1);
		exit(1);
	}

    /* parent */
    waitpid(pid, 0, 0); // sync with execvp
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);

    for (;;) {
        // Enter a syscall
        if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1)
		exit(1);
        if (waitpid(pid, 0, 0) == -1)
		exit(1);

        // Gather syscall arguments
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1)
		exit(1);
        long syscall = regs.orig_rax;

        // Print the monitoring result
        fprintf(stderr, "%ld(%ld, %ld, %ld, %ld, %ld, %ld)",
                syscall,
                (long)regs.rdi, (long)regs.rsi, (long)regs.rdx,
                (long)regs.r10, (long)regs.r8,  (long)regs.r9);

        // Run syscall and stop on exit
        if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1)
		exit(1);
        if (waitpid(pid, 0, 0) == -1)
		exit(1);

        // syscall results
        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {
            fputs(" = ?\n", stderr);
            if (errno == ESRCH)
                exit(regs.rdi); // system call was _exit(2) or similar
	exit(1);
        }

        // Print system call result
        fprintf(stderr, " = %ld\n", (long)regs.rax);
    }
}
