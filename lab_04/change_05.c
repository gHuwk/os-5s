#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

typedef int flag_s;

#define CNT 2
#define SIZE 41

flag_s sigflag = 0;
void sigcatcher(int signum)
{
  printf("\nSignal number %d cathed!\n", signum);
  sigflag = 1;
}

int main()
{
  signal(SIGTSTP, sigcatcher);
  int descr[CNT]; // Дескриптор одного программного канала
  //[0] - выход для чтения, [1] - выход для записи
  // потомок унаследует открытый программный канал предка
  if (pipe(descr) == -1)
  {
    printf("Can`t open canal.\n");
    return 1;
  }

  char result[SIZE];
  pid_t first_childpid, second_childpid;

  first_childpid = fork();
  if (first_childpid == -1)
  {
  // Не был получен потомок
    perror("Can`t fork.\n");
    return 1;
  }
  else if (first_childpid == 0)
  {
    printf("Child first:\n");
    printf("You have 5 second to use Ctrl+Z\n");
    sleep(5);
    
    if (sigflag)
    
    
    
    
    close(descr[0]);
    if (!write(descr[1], "Wow! message first\n", 19))
    {
      printf("Can`t write string\n");
      return 1;
    }
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
    // потомок ничего не прочтет из канала
      close(descr[0]);
      if (!write(descr[1], "Wow! message second\n", 21))
      {
        printf("Can`t write string\n");
        return 1;
      }
      return 0;
    }
   // вот тут предок
    

    sleep(5);

    if (sigflag)
    {
      close(descr[1]);
      if (read(descr[0], result, SIZE_RES) < 0)
      {
        printf("Can`t read string\n");
        return 1;
      }
    }
    else
    {
      strcpy(result,"Time is out!\n\n");
    }

    printf("%s", result);

    int status;
    first_childpid = wait(&status);
    printf("1st child has finished: PID = %d\n", first_childpid);

    if (WIFEXITED(status))
      printf("1st child exited with code %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
      printf("1st child exited with signal number %d\n", WTERMSIG(status));
    else if (WIFSTOPPED(status))
      printf("1st child exited with signal number %d\n", WSTOPSIG(status));

    second_childpid = wait(&status);
    printf("2nd child has finished: PID = %d\n", second_childpid);

    if (WIFEXITED(status))
      printf("2nd child exited with code %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
      printf("2nd child exited with signal number %d\n", WTERMSIG(status));
    else if (WIFSTOPPED(status))
      printf("2nd child exited with signal number %d\n", WSTOPSIG(status));
    return 0;
  }
}
