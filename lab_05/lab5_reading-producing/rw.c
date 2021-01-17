#define _XOPEN_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/shm.h>
int nice(int incr);
 
#define READERS 4
#define WRITERS 4
#define ITER    10

enum SemIds {NReaders, ActiveWriter, MemCapture, WaitingWriters};

void exit(int);

struct sembuf   start_read[] = {{WaitingWriters, 0, 0}, {ActiveWriter, 0, 0}, {NReaders, 1, 0}},
				start_write[] = {{WaitingWriters, 1, 0}, {NReaders, 0, 0}, {ActiveWriter, 1, 0},
								{MemCapture, -1, 0}, {WaitingWriters, -1, 0}},
		
				stop_read[]  = {{NReaders, -1, 0}},
				stop_write[]  = {{ActiveWriter, -1, 0}, {MemCapture, 1, 0}};




int shm_id = -1, sem = -1;
int* shar_mem = NULL;
		
void terminate(int signal){
	printf ("Process %d is halting...\n", getpid());

	while (wait(NULL) != -1) {}
	
	printf ("All child process of process %d finished!\n", getpid());
	
	if (sem != -1)
		semctl(sem, 0, IPC_RMID); 	// deleting semaphore command
	else
		exit(signal);
		
	if (shm_id != -1)
		shmdt(shar_mem); 				// disabling shared memory
	else
		exit(signal);
	
	if (shar_mem != (int *) -1)
		shmctl(shm_id, 0, IPC_RMID); 	// deleting shared memory
	else
		exit(signal);
	
	exit(0);
}

void start_reading (int sem){
	//printf("%d entered start_reading\n", getpid());
	semop(sem, start_read, 3);
	//printf("%d exited start_reading\n", getpid());
	
}

void stop_reading (int sem){
	//printf("%d entered stop_reading\n", getpid());
	semop(sem, stop_read, 1);
	//printf("%d exited stop_reading\n", getpid());
}

void start_writing(int sem){
	//printf("%d entered start_writing\n", getpid());
	semop(sem, start_write, 5);
	//printf("%d exited start_writing\n", getpid());
}

void stop_writing(int sem){
	//printf("%d entered stop_writing\n", getpid());
	semop(sem, stop_write, 2);
	//printf("%d exited stop_writing\n", getpid());
	//semop(sem, stop_write_r, 1);
	//semop(sem, stop_write_w, 1);
}

void reader(int sem, int* shar_mem, int num){
	for (int i = 0; i < ITER; ++i){
		//for(int i = 0; i < 0xfff; ++i);
		start_reading(sem);
		printf ("Reader %d have read %d from shared memory!\n", num, *shar_mem);
		stop_reading(sem);
		nice(15);
	}
	
	exit(0);
}

void writer (int sem, int* shar_mem, int num){
	for (int i = 0; i < ITER; ++i){
		//for(int i = 0; i < 0xfff; ++i);
		start_writing(sem);
		printf("Writer %d have changed value in shared memory to: %d\n", num, ++(*shar_mem));
		stop_writing(sem);
		nice(15);
	}
	
	exit(0);
}

int main(){
	int perms = IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO;
	
	if ((sem = semget(IPC_PRIVATE, 4, perms)) == -1){
        perror("Semget error \n"); 
        terminate(1);
    }

	semctl(sem, NReaders, SETVAL, 0);
    semctl(sem, ActiveWriter, SETVAL, 0);
    semctl(sem, MemCapture, SETVAL, 1);
    semctl(sem, WaitingWriters, SETVAL, 0);
	
	if ((shm_id = shmget(IPC_PRIVATE, sizeof(int), perms)) == -1){
		perror ("Shmget error!\n");
		terminate(2);
	}
	
	if ((shar_mem = (int *) shmat(shm_id, 0, 0)) == NULL){
		perror ("Shmat error!\n");
		terminate(3);
	}
	
	*shar_mem = 0;
	
	signal(SIGABRT, &terminate);
    signal(SIGTERM, &terminate);
    signal(SIGINT, &terminate); 
    signal(SIGKILL, &terminate);

	pid_t pid;
	
	for (int i = 0 ; i < WRITERS; ++i)
		if ((pid = fork()) != -1)
			if (!pid)
				writer(sem, shar_mem, i);
			else
				;
		else
			perror ("Fork error while creating writers!\n");
			
		
	for (int i = 0; i < READERS; ++i)
		if ((pid = fork()) != -1)
			if (!pid)
				reader(sem, shar_mem, i);
			else
				;
		else
			perror ("Fork error while creating readers!\n");
	
	terminate(0);
}
