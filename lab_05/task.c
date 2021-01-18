#include <stdio.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/wait.h>

// Длина буфера
#define CNT_BUF 10
#define VALUES 10
// количество производителей и потребителей
#define CNT_PROD 4
#define CNT_CONS 4

// индентификаторы семафоров в наборе
#define BIN_SEM_NUM 0
#define BUFFER_EMPTY_NUM 1
#define BUFFER_FULL_NUM 2

// перед входом в критическую секцию производит
struct sembuf producer_P[] = {{BUFFER_EMPTY_NUM, -1, 0},{BIN_SEM_NUM, -1, 0}};
struct sembuf producer_V[] = {{BIN_SEM_NUM, 1, 0},{BUFFER_FULL_NUM, 1, 0}};
struct sembuf consumer_P[] = {{BUFFER_FULL_NUM, -1, 0},{BIN_SEM_NUM, -1, 0}};
struct sembuf consumer_V[] = {{BIN_SEM_NUM, 1, 0},{BUFFER_EMPTY_NUM, 1, 0}};

// Адрес сегмента памяти
char *shared_buffer;
char *shared_pos_consumer;
char *shared_pos_producer;
char *shared_letter;

int producer(int n, int semaphore)
{
    //pid_t pr_pid = getpid();
    int where = *shared_pos_producer;
    while(1)
    {

        //printf("HERE! PROD %d\n", pr_pid);
        semop(semaphore, producer_P, sizeof(char));
        shared_buffer[*shared_pos_producer] = 'a' + *shared_letter;
        *(shared_letter) = (*(shared_letter) + 1);
        printf("\x1b[7mPROD %d PID = %d produce %c\n", n, 1, shared_buffer[*shared_pos_producer]);
        //*(shared_pos_producer) = (*(shared_pos_producer) + 1);
        where = (where + 1) % CNT_BUF;
        semop(semaphore, producer_V, sizeof(char));
        sleep(1);
    }
    //return 0;
}

int consumer(int n, int semaphore)
{
    //pid_t co_pid = getpid();
    int where = *shared_pos_consumer;
    while(1)
    {

        //printf("HERE! CONS! %d\n", co_pid);
        semop(semaphore, consumer_P, 2);
        printf("\x1b[0mCONS %d PID = %d consume %c\n", n, 2, shared_buffer[*shared_pos_consumer]);
        //(*shared_pos_consumer) = (*shared_pos_consumer) + 1;
        where = (where + 1) % CNT_BUF;
        semop(semaphore, consumer_V, 2);
        sleep(2);
    }
    //return 0;
}

int main()
{

    int permissions = S_IRWXU | S_IRWXG | S_IRWXO;
    // Выделяем память
    int palace = shmget(IPC_PRIVATE, (CNT_BUF + 1) * sizeof(char) + 3 * sizeof(char), IPC_CREAT | permissions);
    if (palace == -1)
    {
        perror("Can't shmget!\n");
        exit(1);
    }
    // Активируем совместный доступ
    shared_pos_producer = shmat(palace, NULL, 0);
    if (*shared_pos_producer == -1)
    {
        perror("Can't shmat!\n");
        exit(1);
    }
    // Смещение

    shared_buffer = shared_pos_producer + 3 * sizeof(char);
    shared_pos_consumer = shared_pos_producer + sizeof(char);
    shared_letter = shared_pos_producer + 2 * sizeof(char);
    (*shared_pos_producer) = 0;
    (*shared_pos_consumer) = 0;
    (*shared_letter) = 0;

    // Набор семафоров
    int semaphores = semget(IPC_PRIVATE, 3, permissions);
    if (semaphores == -1)
    {
        perror("Can't semget!\n");
        exit(1);
    }

    // Контроль за семафорами
    int ctl_bin = semctl(semaphores, BIN_SEM_NUM, SETVAL, 1);
    int ctl_empty = semctl(semaphores, BUFFER_EMPTY_NUM, SETVAL, CNT_BUF);
    int ctl_full = semctl(semaphores, BUFFER_FULL_NUM, SETVAL, 0);

    if (ctl_bin == -1 || ctl_empty == -1 || ctl_full == -1)
    {
        perror("Can't semctl!");
        exit(1);
    }

    // создание процессов производителей
    for (int i = 0; i < CNT_PROD; i++)
    {
        //printf("Create %d\n", i);
        pid_t pid;
        if ((pid = fork()) == -1)
        {
            perror("Can't fork prod!\n");
            exit(1);
        }
        //printf("Created %d\n", i);
        if (pid == 0)
        {
            producer(i + 1, semaphores);
            return 0;
        }
    }

    for (int i = 0; i < CNT_CONS; i++)
    {
        pid_t pid;
        if ((pid = fork()) == -1)
        {
            perror("Can't fork cons!\n");
            exit(1);
        }

        if (pid == 0)
        {
            consumer(i + 1, semaphores);
            return 0;
        }
    }

    for (int i = 0; i < CNT_PROD + CNT_CONS; i++)
    {
        // Дожидаемся заврещения процессов потомков
        printf("wait!\n");
        int status;
        wait(&status);
    }

    if (shmdt(shared_pos_producer) == -1)
    {
        perror("Can't shmdt!\n");
        exit(1);
    }
    printf("10\n");
    return 0;
}
