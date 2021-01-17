#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
  pid_t first_childpid, second_childpid;
  first_childpid = fork();

  if (first_childpid == -1)
  {
    perror("Can`t fork.\n");
    return 1;
  }
  else if (first_childpid == 0)
  {
    printf("\n1st child proc:\n");
    printf("\tPID: %d\n", getpid());
    printf("\tPPID: %d\n", getppid());
    printf("\tPGRP: %d\n", getpgrp());
    printf("Current PS:\n");
    execlp("ps", "", NULL);
    return 0;
  }
  else
  {
    int status;

    first_childpid = wait(&status);
    printf("1st child has finished: PID = %d\n", first_childpid);

    if (WIFEXITED(status))
      printf("1st child exited with code %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
      printf("1st child exited with signal number %d\n", WTERMSIG(status));
    else if (WIFSTOPPED(status))
      printf("1st child exited with signal number %d\n", WSTOPSIG(status));

    if ((second_childpid = fork()) == -1)
    {
      perror("Can`t fork.\n");
      return 1;
    }
    else if (second_childpid == 0)
    {
      printf("2nd child proc:\n");
      printf("\tPID: %d\n", getpid());
      printf("\tPPID: %d\n", getppid());
      printf("\tPGRP: %d\n", getpgrp());

      printf("OS:\n");
      execlp("neofetch", "", NULL);

      return 0;
    }

    second_childpid = wait(&status);
    printf("2nd child has finished: PID = %d\n", second_childpid);

    if (WIFEXITED(status))
      printf("2nd child exited with code %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
      printf("2nd child exited with signal number %d\n", WTERMSIG(status));
    else if (WIFSTOPPED(status))
      printf("2nd child exited with signal number %d\n", WSTOPSIG(status));

    printf("Parent proc:\n");
    printf("\tPID: %d\n", getpid());
    printf("\tPID_FIRST: %d\n\tPID_SECOND %d\n", first_childpid, second_childpid);
    printf("\tPGRP: %d\n", getpgrp());

    return 0;
  }
}
