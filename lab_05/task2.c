#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>


#define READERS 5
#define WRITERS 3



#define SEM_CNT(a) sizeof(a) / sizeof(struct sembuf)

const int COUNT = 10;
const int PERMISSIONS = S_IRWXU | S_IRWXG | S_IRWXO;

struct sembuf start_read[] = {{0, 1, 0},
                               {1, 0, 0},
                               {2, 0, 0},
                               {3, 1, 0},
                               {0, -1, 0}};

struct sembuf stop_read[] = {{3, -1, 0}};

struct sembuf start_write[] = {
    {2, 1, 0},
    {3, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {2, -1, 0}
};

struct sembuf stop_write[] = {{1, -1, 0}};

void writer(int semophore, int *shm)
{
    int value = 0;
    while (value < COUNT)
    {
        if (semop(semophore, start_write, 5))
        {
            perror("semop error");
            exit(1);
        }

        (*shm)++;
        printf("Writer PID: %d write %d\n", getpid(), *shm);

        if (semop(semophore, stop_write, 1))
        {
            perror("semop error");
            exit(1);
        }

        value++;

        sleep(rand() % 5);
    }
}

void reader(int semophore, int *shm)
{
    int value = 0;
    while (value < COUNT)
    {
        if (semop(semophore, start_read, 5))
        {
            perror("semop error");
            exit(1);
        }

        printf("Reader PID: %d read %d\n", getpid(), *shm);

        if (semop(semophore, stop_read, 1))
        {
            perror("semop error");
            exit(1);
        }

        value++;
        sleep(rand() % 5);
    }
}

int main()
{
    int shm_id;
    printf("HEY!");
    if ((shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | PERMISSIONS)) == -1)
    {
        perror("Can't SHMGET!\n");
        exit(1);
    }

    int *shm_buf = shmat(shm_id, 0, 0);
    if (shm_buf == -1)
    {
        perror("Can't SHMAT!\n");
        exit(1);
    }

    (*shm_buf) = 0;

    int sem_id;
    if ((sem_id = semget(IPC_PRIVATE, sizeof(int), IPC_CREAT | PERMISSIONS)) == -1)
    {
        perror("Can't SEMGET!\n");
        exit(1);
    }

    // изменение управляющих параметров набора семафоров
    semctl(sem_id, 0,  SETVAL, 0);
    semctl(sem_id, 1,   SETVAL, 0);
    semctl(sem_id, 2, SETVAL, 0);
    semctl(sem_id, 3, SETVAL, 0);

    pid_t pid;
    for (int i = 0; i < WRITERS && pid != 0; i++)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("Can't FORK writers!\n");
            exit(1);
        }
        if (pid == 0)
        {
            writer(sem_id, shm_buf);
        }
    }

    for (int i = 0; i < READERS && pid != 0; i++)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("Can't FORK readers!\n");
            exit(1);
        }
        if (pid == 0)
        {
            reader(sem_id, shm_buf);
        }
    }

    if (pid != 0)
    {
        int *status;
        for (int i = 0; i < READERS + WRITERS; ++i)
        {
            wait(status);
        }

        if (shmdt(shm_buf) == -1)
        {
            perror("Can't SHMDT!\n");
            exit(1);
        }

        if (shmctl(shm_id, IPC_RMID, NULL) == -1)
        {
            perror("Can't SHMCTL!\n");
            exit(1);
        }
    }

    return 0;
}
