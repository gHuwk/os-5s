#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define COUNT 3
#define PROD 0
#define CONS 1

#define PERMS S_IRWXU | S_IRWXG | S_IRWXO

#define EMPTYCOUNT 0
#define FULLCOUNT 1
#define BIN 2

int semaphore;
int shared_memory;
char **addr_shared_memory;

// Массив структур sem
// semop = -1 - я использую общий ресурс, остановите меня, если он занят
//       = 1 - я закончил работу и ресурс свободен, честное слово
// SEM_UNDO отменяет операцию над семафором, если процесс неожиданно завершается

struct sembuf producer_grab[2] = {
    {EMPTYCOUNT, -1, SEM_UNDO},
    {BIN, -1, SEM_UNDO}
};

struct sembuf producer_free[2] = {
    {BIN, 1, SEM_UNDO},
    {FULLCOUNT, 1, SEM_UNDO}
};

struct sembuf consumer_grab[2] = {
    {FULLCOUNT, -1, SEM_UNDO},
    {BIN, -1, SEM_UNDO}
};

struct sembuf consumer_free[2] = {
    {BIN, 1, SEM_UNDO},
    {EMPTYCOUNT, 1, SEM_UNDO}
};

// Опишем типичного потребителя
void consumer(int semaphore, int value)
{
    while(1)
    {
        sleep(1);
        int sem_op_p = semop(semaphore, consumer_grab, 2);
        if (sem_op_p == -1)
        {
            perror("Can't semop\n");
            exit(1);
        }

        if(
        (char*)(*(addr_shared_memory + sizeof(int *))) ==
        ((char*)(addr_shared_memory) + 2*sizeof(int*) + 5 * sizeof(int)))
        {
            *(addr_shared_memory + sizeof(int*)) = (char*)addr_shared_memory + 2 * sizeof(int*);
        }

        printf("Consumer %d get %d\n", value, **(addr_shared_memory + sizeof(int*)));
        (*(addr_shared_memory + sizeof(int*)))++;

        int sem_op_v = semop(semaphore, consumer_free, 2);
        if (sem_op_v == -1)
        {
            perror("Can't semop \n");
            exit(1);
        }
    }
}

void producer(int semaphore, int value)
{
    while(1)
    {
        sleep(2);
        int sem_op_p = semop(semaphore, producer_grab, 2);
        if (sem_op_p == -1)
        {
            perror("Can't semop \n");
            exit(1);
        }

        if ((char*)(*addr_shared_memory) == ((char*)(addr_shared_memory)+ 2 *sizeof(int*) + 5 * sizeof(int)))
        {
            (*addr_shared_memory)= (char*)addr_shared_memory + 2 * sizeof(int*);
        }
        *(*addr_shared_memory) = ((char*)(*addr_shared_memory) - (char*)addr_shared_memory) - 16;
        printf("Producer %d put %d\n", value, *(*addr_shared_memory));
        (*addr_shared_memory)++;

        int sem_op_v = semop(semaphore, producer_free, 2);
        if (sem_op_v == -1)
        {
            perror("Cant semop \n");
            exit(1);
        }
    }
}

int main()
{
    int process;
    int consumers[3];
    int producers[3];
    // Создаем семафор
    semaphore = semget(IPC_PRIVATE, 3, IPC_CREAT | PERMS);
    int se = semctl(semaphore, EMPTYCOUNT, SETVAL, COUNT);
    int sf = semctl(semaphore, FULLCOUNT, SETVAL, 0);
    int sb = semctl(semaphore, BIN, SETVAL, 1);
    // Объявляем сегмент разделяемоей памяти
    shared_memory = shmget(IPC_PRIVATE, 2 * sizeof(int*) + 5 * sizeof(int), IPC_CREAT | PERMS);
    addr_shared_memory = shmat(shared_memory, NULL, 0);

    *(addr_shared_memory) = (char*)addr_shared_memory + 2 * sizeof(int*);
    *(addr_shared_memory + sizeof(int *)) = (char*)addr_shared_memory + 2* sizeof(int*);

    // Создание процессов
    for (int i = 0; i < COUNT; i++)
    {
        if (-1 == (producers[i] = fork()))
        {
            return 1;
        }
        else if(0 == producers[i])
        {
            producer(semaphore, i);
            exit(0);
        }

        if(-1 == (consumers[i] = fork()))
        {
            return 1;
        }
        else if (0 == consumers[i])
        {
            consumer(semaphore, i);
            exit(0);
        }
    }

    //signal(SIGINT, catch_sigp);
    //int status;
    //wait(&status);

    // Очистка памяти
    shmctl(shared_memory, IPC_RMID, NULL);
    semctl(semaphore, 0, IPC_RMID, 0);
    return 0;
}


