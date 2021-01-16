#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
  int first_childpid, second_childpid;

  first_childpid = fork();
  if (first_childpid == -1)
  {
    perror("Can`t fork.\n");
    return 1;
  }
  else if (first_childpid == 0)
  {
    sleep(1);

    printf("\n1st child proc:\n");
    printf("\tPID: %d\n", getpid());
    printf("\tPPID: %d\n", getppid());
    printf("\tPGRP: %d\n", getpgrp());

    return 0;
  }
  else
  {
    if ((second_childpid = fork()) == -1)
    {
      perror("Can`t fork.\n");
      return 1;
    }
    else if (second_childpid == 0)
    {
      sleep(2);

      printf("2nd child proc:\n");
      printf("\tPID: %d\n", getpid());
      printf("\tPPID: %d\n", getppid());
      printf("\tPGRP: %d\n", getpgrp());

      return 0;
    }

    printf("Parent proc:\n");
    printf("\tPID: %d\n", getpid());
    printf("\tPID_FIRST: %d\n\tPID_SECOND %d\n", first_childpid, second_childpid);
    printf("\tPGRP: %d\n", getpgrp());

    return 0;
  }
}

