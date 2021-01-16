#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CNT 2
#define SIZE_RES 41
#define SIZE_MSG_F 17
#define SIZE_MSG_S 19


int main()
{
  int descr[CNT];

  if (pipe(descr) == -1)
  {
    printf("Can`t pipe\n");
    return 1;
  }

  char result_data[SIZE_RES];

  pid_t second_childpid;
  pid_t first_childpid = fork();
  
  if (first_childpid == -1)
  {
    perror("Can`t fork.\n"); 
    return 1;        
  }
  else if (first_childpid == 0)
  {
    close(descr[0]);
    if (!write(descr[1], "Bark! from first\n", SIZE_MSG_F))
    {  
      printf("Can`t write string\n");
      return 1;
    }

    return 0;
  }
  else
  {
    int status;
    first_childpid = wait(&status);
    printf("1st child has finished:\n\tPID = %d\n", first_childpid); 

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
      close(descr[0]);
      if (!write(descr[1], "Bark! from second\n", SIZE_MSG_S))
      {  
        printf("Can`t write string\n");
        return 1;
      }

      return 0;
    }

    second_childpid = wait(&status);
    printf("2nd child has finished:\n\tPID = %d\n", second_childpid);
        
    if (WIFEXITED(status))
      printf("2nd child exited with code %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
      printf("2nd child exited with signal number %d\n", WTERMSIG(status)); 
    else if (WIFSTOPPED(status))
      printf("2nd child exited with signal number %d\n", WSTOPSIG(status));   

    close(descr[1]);
    if (read(descr[0], result_data, SIZE_RES) < 0)
    {  
      printf("Can`t read string\n");
      return 1;
    }

    printf("%s", result_data);
    
    return 0;
  }
}
