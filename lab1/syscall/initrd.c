	#define _GNU_SOURCE
       	#include <unistd.h>
       	#include <sys/syscall.h>
       	#include <sys/types.h>
       	#include <signal.h>
	#include <stdlib.h>
	#include <stdio.h>

       	int main(int argc, char *argv[])
        {
	   
	   char *p = (char*)malloc(10*sizeof(char));

	   int x = syscall(548, p, 10);	 // not enough
	   
	   if( x != -1 )
	   	printf("Error!\n");
	   else
		printf("Return Value = %d\n", x);

	   printf("\n");
	   
	   char *q = (char*)malloc(20*sizeof(char));

	   int y = syscall(548, q, 20);	 // enough

	   if( y )
	   	printf("Error!\n");
	   else
		printf("Saved String: %s", q);

	   while( 1 ) { }
     	}
