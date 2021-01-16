#include <stdio.h>
int main()
{
	int childpid;
	if ((childpid =fork())==-1)
	{
		perror("I cant fork it.\n");
		return 1;
	}
	else if (childpid == 0)
	{
		while(1) printf("%d\n", getpid());
		return 0;
	}
	else
	{
		while(1) printf("%d\n", getpid());
		return 0;
	}
}
