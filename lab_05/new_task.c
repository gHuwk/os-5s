#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CNT_BUF 5
#define VALUES 10

#define PROD 3
#define CONS 3

#define BIN 0
#define EMPTY 1
#define FULL 2

struct sembuf producer_P[2] = {{EMPTY, -1, 0}, {FULL, -1, 0}};
struct sembuf producer_V[2] = {{BIN, 1, 0}, {FULL, 1, 0}};
struct sembuf consumer_P[2] = {{BIN, -1, 0}, {FULL, -1, 0}};
struct sembuf consumer_V[2] = {{EMPTY, 1, 0}, {FULL, 1, 0}};

int *shared_buffer;
int *shared_pos_consumer;
int *shared_pos_producer;
int *shared_letter;

int producer(int semaphores, pid_t pid)
{
    while (1)
    {
        sleep(rand() % 5);
        if (semop(semaphores, producer_P, 2) == -1)
            {
                perror("Can't SEMOP!\n");
                exit(1);
            }

        shared_buffer[*shared_pos_producer] = *shared_letter + 'a';
        printf("\x1b[7mPROD #%d: %c  to  buf[%d]\n", pid, shared_buffer[*shared_pos_producer], *shared_pos_producer);
        (*shared_pos_producer) = (*shared_pos_producer + 1) % CNT_BUF;

        (*shared_letter) = (*shared_letter + 1) % 26;

        if (semop(semaphores, producer_V, 2) == -1)
            {
                perror("Can't SEMOP!\n");
                exit(1);
            }
    }
    return 0;
}

int consumer(int semaphores, pid_t pid)
{
    while (1)
    {
        sleep(rand() % 2);
        if (semop(semaphores, consumer_P, 2) == -1)
            {
                perror("Can't SEMOP!\n");
                exit(1);
            }

        printf("\x1b[0mCONS #%d: %c from buf[%d]\n", pid, shared_buffer[*shared_pos_consumer], *shared_pos_consumer);
        (*shared_pos_consumer) = (*shared_pos_consumer + 1) % CNT_BUF;

        if (semop(semaphores, consumer_V, 2) == -1)
            {
                perror("Can't SEMOP!\n");
                exit(1);
            }
    }
    return 0;
}

int main()
{
    srand(NULL);
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;

    int fd = shmget(IPC_PRIVATE, (CNT_BUF + 1) * sizeof(char), IPC_CREAT | perms);
    if (fd == -1)
    {
        perror("Can't SHMGET!\n");
        exit(1);
    }

    shared_pos_producer = shmat(fd, 0, 0);
    if (*shared_pos_producer == -1)
    {
        perror("Can't SHMAT!\n");
        exit(1);
    }

    shared_buffer = shared_pos_producer + 3 * sizeof(char);
    shared_pos_consumer = shared_pos_producer + 2 * sizeof(char);
    shared_letter = shared_pos_producer + sizeof(char);
    (*shared_pos_producer) = 0;
    (*shared_pos_consumer) = 0;
    (*shared_letter) = 0;

    int semaphores = semget(IPC_PRIVATE, 3, IPC_CREAT | perms);
    if (semaphores == -1)
        {
            perror("Can't SEMGET!\n");
            exit(1);
        }

    int ctlb = semctl(semaphores, BIN, SETVAL, 0);
    int ctle = semctl(semaphores, EMPTY, SETVAL, CNT_BUF);
    int ctlf = semctl(semaphores, FULL, SETVAL, 1);

    if (ctlf == -1 || ctle == -1 || ctlb == -1)
    {
        perror("Can't SEMCTL!\n");
        exit(1);
    }

    pid_t pid;
    for (int i = 0; i < PROD && pid != 0; i++)
    {
        if ((pid = fork()) == -1)
        {
            perror("Can't FORK prod!\n");
            exit(1);
        }
        if (pid == 0)
        {
            producer(semaphores, getpid());
            return 0;
        }
    }

    for (int i = 0; i < CONS && pid != 0; i++)
    {

        if ((pid = fork()) == -1)
        {
            perror("Can't FORK cons!\n");
            exit(1);
        }
        if (pid == 0)
        {
            consumer(semaphores, getpid());
            return 0;
        }
    }

    if (pid != 0)
    {
        int status;
        for (int i = 0; i < CNT_BUF; i++)
            wait(&status);

        if (shmdt(shared_pos_producer) == -1)
        {
            perror("Can't SHMDT!\n");
            exit(1);
        }
    }
    return 0;
}
