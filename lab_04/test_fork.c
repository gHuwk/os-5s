#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
	pid_t child_pid = fork(); // Если форк завершился успешно, pid > 0 в родительском процессе.
	if (child_pid == -1)
	{
		perror("Can't fork"); // fork не удался. Скоее всего что-то с памятью.
		exit(1);
	}
	else if (child_pid == 0)
	{
		// child code
		sleep(1);
		printf("Hello from child!\n");
		printf("%d - current pid\n", child_pid);
		return 0;
	}
	// parent code
	int status;
	pid_t childpid;
	child_pid = wait(&status);
	printf("Child has finished: PID = %d\n", childpid);
	if (WIFEXITED(status))
		printf("Child exited with code %d\n", WEXITSTATUS(status));
	else
		printf("Child terminated abnormally\n");
	sleep(2);
	printf("Hello from parent!\n");
	printf("%d - current pid\n", child_pid);
	return 0;
}
