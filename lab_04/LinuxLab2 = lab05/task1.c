/*
* ???????? ?????????, ??????????? ????? ??????? ????????? ??????? fork()
* ? ?????? ??????? ??????????? ?????????????? ( ??????? getpid()),
*					?????????????? ?????? (??????? getgrp()),
*                   ?????????????? ???????.
* ? ??????? ??????? ??????????? ??????????????,
* 					?????????????? ??????,
*					?????????????? ??????
* ?????????, ??? ??? ?????????? ????????-?????? ??????? ???????? ?????????????? ?????? (PPID = 1)
*/
#include <stdio.h> 	//printf
#include <stdlib.h> //exit
#include <unistd.h> // accept POSIX standarts


int main()
{
	int child = fork();
	if ( child == -1 )
	{
		perror("Error: fork().");
		exit(1);
	}
	if ( child == 0 ) // child
	{
		sleep(1);
		printf( "\nChild-proc:\n\tpid=%d\n\tgroup=%d\n\tparent=%d\n", getpid(), getpgrp(), getppid() );
		return 0;
	}
	else			 // parent
	{
        printf( "\nParent-proc:\n\tpid=%d\n\tgroup=%d\n\tchild=%d\n", getpid(), getpgrp(), child );
		return 0;
	}
}