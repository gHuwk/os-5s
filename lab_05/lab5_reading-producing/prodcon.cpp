#include <sys/types.h> // aparently isn't used for anything
#include <sys/wait.h> //wait()

//#include <sys/ipc.h> // aparently isn't used for anything
#include <sys/shm.h> //shmat(), shmget()

#include <sys/sem.h> //semgop(), semget()

#include <stdio.h> //printf()
#include <unistd.h> //fork(), getpid()
#include <stdlib.h> //exit(), rand()

#include <time.h> //nanosleep()

#define MAX_PROC 2 // count of processes to fork
#define MAX_CONVEYOR 5 // max count of items on conveyor
#define MAX_ACTIONS 7 // count of actions each forked process will do

#define FORK_CONSUMER 0
#define FORK_PRODUCER 1

#define SEM_ACT_WAIT 0
#define SEM_ACT_MINUS -1
#define SEM_ACT_PLUS 1

double sleepC = 0.0;
double sleepP = 0.0; // time each process sleeps after doing operation

key_t shm_key = 1234; // key to get/init shm
int shm_flg = 0666; // flags of access to shm
int shm_size = sizeof(int) * MAX_CONVEYOR; // size of shm block
int shm_id = 0; // id of acquired shm

// returns offset to block in shared-heap memory
char* GetSHM();

key_t sem_key = 5678; // key to get/init semaphore
int sem_flg = 0666; // flags of access to semaphore
int sem_cnt = 2; // count of semaphores in set
int sem_id = -1; // id of acquired semaphore set

#define SEM__COUNT 3
#define SEM_MEMACCESS 0
#define SEM_EMPTYCOUNT 1
#define SEM_FULLCOUNT 2

sembuf opStartProduce[] = {		{SEM_EMPTYCOUNT, -1, 0},	{SEM_MEMACCESS, -1, 0}	};
sembuf opEndProduce[] = {		{SEM_MEMACCESS, 1, 0},		{SEM_FULLCOUNT, 1, 0}	};

sembuf opStartConsume[] = {		{SEM_FULLCOUNT, -1, 0},		{SEM_MEMACCESS, -1, 0}	};
sembuf opEndConsume[] = {		{SEM_MEMACCESS, 1, 0},		{SEM_EMPTYCOUNT, 1, 0}	};

// acquires semaphore set
int GetSem();

// initializes semaphore set' values
void InitSem();

// enables sleep for reals instead of integers
void sleep(double period)
{
	time_t s = (time_t)period;
	double ns = (period - s) * 1e9;
	timespec t = { s, (long)ns };
	//printf( "waitnig: %lf;  %d + %lf    /    %d + %d\n", period, s, ns, t.tv_sec, t.tv_nsec );
	nanosleep( &t, NULL );
}



void Consume()
{
	int pos;
	int sem = GetSem();
	int *conv = (int*)GetSHM();
	for (int i=0; i<MAX_ACTIONS; i++)
	{
		printf( "[- con...\n" );
		semop( sem, opStartConsume, 2 );
		
		pos = semctl( sem, SEM_FULLCOUNT, GETVAL );
		printf( " -]con: reading %d from %d pos.\n", conv[pos], pos );
		
		semop( sem, opEndConsume, 2 );
		sleep( sleepC );
	}
	printf( " * con: finished.\n" );
}

void Produce()
{
	int pos;
	int sem = GetSem();
	int *conv = (int*)GetSHM();
	for (int i=0; i<MAX_ACTIONS; i++)
	{
		printf( "[+ prod...\n" );
		semop( sem, opStartProduce, 2 );
		
		pos = semctl( sem, SEM_FULLCOUNT, GETVAL );
		conv[pos] = i + 10;
		printf( " +]prod: writing %d to %d pos.\n", conv[pos], pos );
		
		semop( sem, opEndProduce, 2 );
		sleep( sleepP );
	}
	printf( " * prod: finished.\n" );
}



int main(int argc, char* argv[])
{	
	int prnt = getpid();
	int chlds[MAX_PROC];
	
	InitSem();
	
	int *conveyor = (int*)GetSHM();
	
	if ( argc > 1 )
	{
		sscanf( argv[1], "%lf", &sleepC );
		sscanf( argv[2], "%lf", &sleepP );
	}	
	
	for (int i=0; i<MAX_PROC; i++)
	{
		chlds[i] = fork();
		if ( getpid() != prnt ) // in children procs - cleansing previous call (if can) and immidiately leaving cycle
		{
			for (int j=0; j<i; j++)
				chlds[j] = -1;
			break;
		}
				
		printf( " * main: startnig %d ", chlds[i] );
		switch (i)
		{
			case FORK_PRODUCER:
				printf( "(prod)\n" );
				break;
			case FORK_CONSUMER:
				printf( "(con)\n" );
				break;
		}
	}
	
	// starting mainloops in children procs
	for (int i=0; i<MAX_PROC; i++)
	{
		if ( chlds[i] != 0 ) // skipping the cycle in main program and incorrect children
			continue;
		if ( i == FORK_CONSUMER )
		{
			Consume();
			return 0;
		}
		
		if ( i == FORK_PRODUCER )
		{
			Produce();
			return 0;
		}
	}
	
	if ( getpid() == prnt )
	{
		for (int i=0; i<MAX_PROC; i++)
			wait(NULL);
	}
	
	return 0;
}



inline char* GetSHM()
{
	// creating the segment
	if ( !shm_id )
	{
		shm_id = shmget( shm_key, shm_size, shm_flg | IPC_CREAT );
		if ( shm_id < 0 )
		{
			perror( "shmget" );
			exit( 1 );
		}
	}
	
	// attaching the segment
	char *shm = (char*)shmat( shm_id, NULL, 0 );
	if ( shm == (char*)-1 )
	{
		perror( "shmat" );
		exit( 2 );
	}
	
	return shm;
}

inline int GetSem()
{
	int sem = semget( sem_key, sem_cnt, sem_flg );
	
	if ( sem == -1 )
	{
		perror( "semget (get)" );
		exit( 5 );
	}
	
	return sem;
}

void InitSem()
{						//	mem,	empty,		full
	unsigned short vals[] = { 1, MAX_CONVEYOR, 0 };
	int c = semctl( sem_id, 0, SETALL, vals );
	
	if ( c == -1 )
	{
		perror( "init semctl" );
		exit( 7 );
	}
}
