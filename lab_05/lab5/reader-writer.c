#include <stdio.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/wait.h>

// Количество читателей и писателей
#define COUNT_READERS 4
#define COUNT_WRITERS 4

// Операции на семафорах
#define SEM_OP_INC 1
#define SEM_OP_DEC -1
#define SEM_OP_WAIT 0

// Ключи
#define ACTIVE_READERS 0
#define ACTIVE_WRITER 1
#define WAITING_WRITERS 2
#define WAITING_READERS 3


struct sembuf start_writer[] = {{WAITING_WRITERS,   SEM_OP_INC, 0},
                                {ACTIVE_READERS,    SEM_OP_WAIT, 0},
                                {ACTIVE_WRITER,     SEM_OP_WAIT, 0},
                                {ACTIVE_WRITER,     SEM_OP_INC, 0},
                                {WAITING_WRITERS,   SEM_OP_DEC, 0},},
               stop_writer[] = {{ACTIVE_WRITER, -1, 0}},
              start_reader[] = {{WAITING_READERS,   SEM_OP_INC, 0},
                                {WAITING_WRITERS,   SEM_OP_WAIT, 0},
                                {ACTIVE_WRITER,     SEM_OP_WAIT, 0},
                                {ACTIVE_READERS,    SEM_OP_INC, 0},
                                {WAITING_READERS,   SEM_OP_DEC, 0},},
               stop_reader[] = {{ACTIVE_READERS, -1, 0}};


void Reader(int nomer, int sem_id, int* buf)
{
    while (1)
    {
        semop(sem_id, start_reader, 5);

        printf("Reader #%d -> ", nomer);
        printf("read %d\n", *buf);

        semop(sem_id, stop_reader, 1);

        sleep(rand() % 2);
    }
}

void Writer(int nomer, int sem_id, int* buf)
{
    while (1)
    {
        semop(sem_id, start_writer, 5);

        printf("Writer #%d -> ", nomer);
        printf("write %d\n", ++(*buf));

        semop(sem_id, stop_writer, 1);

        sleep(rand() % 3);
    }
}

int main()
{
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    int shm_id;
    int sem_id;
    int *mem_ptr;

    // возвращает идентификатор разделяемому сегменту памяти
    if ((shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | perms)) == -1)
    {
        perror("Unable to create a shared area.\n");
        return 1;
    }

    // возвращает указатель на сегмент разделяемой памяти
    if ((mem_ptr = shmat(shm_id, NULL, 0)) == -1)
    {
        perror("Can't attach memory.\n");
        return 1;
    }

    // создание набора семафоров
    if ((sem_id = semget(IPC_PRIVATE, 4, perms)) == -1)
    {
        perror("Can’t semget.\n");
        return 1;
    }

    // изменение управляющих параметров набора семафоров
    semctl(sem_id, ACTIVE_READERS,  SETVAL, 0);
    semctl(sem_id, ACTIVE_WRITER,   SETVAL, 0);
    semctl(sem_id, WAITING_READERS, SETVAL, 0);
    semctl(sem_id, WAITING_WRITERS, SETVAL, 0);

    // создание процессов
    int count_processes = 0;
    pid_t pid;

    *mem_ptr = 0;

    for (int i = 0; i < COUNT_WRITERS; i++)
    {
        if ((pid = fork())== -1)
        {
            perror("Can’t fork.\n");
            return 1;
        }

        if (!pid)
            Writer(i + 1, sem_id, mem_ptr);
        else
            count_processes++;
    }

    for (int i = 0; i < COUNT_READERS; i++)
    {
        if ((pid = fork())== -1)
        {
            perror("Can’t fork.\n");
            return 1;
        }

        if (!pid)
            Reader(i + 1, sem_id, mem_ptr);
        else
            count_processes++;
    }

    // ожидание завершение процессов
    int status;
    for (int i = 0; i < count_processes; i++)
    {
        wait(&status);
        if (!WIFEXITED(status))
            printf("exit-error, code = %d\n", status);
    }

    // освобождение ресурсов
    shmdt(mem_ptr);
    semctl(sem_id, 0, IPC_RMID);
    shmctl(shm_id, IPC_RMID, 0);

    return 0;
}
