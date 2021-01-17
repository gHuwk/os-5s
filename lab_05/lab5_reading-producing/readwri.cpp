#include <sys/types.h> // aparently isn't used for anything
#include <sys/wait.h> //wait()

//#include <sys/ipc.h> // aparently isn't used for anything
#include <sys/shm.h> //shmat(), shmget()

#include <sys/sem.h> //semgop(), semget()

#include <stdio.h> //printf()
#include <string.h> //memset()
#include <unistd.h> //fork(), getpid()
#include <stdlib.h> //exit(), rand()

#include <time.h> //nanosleep()





#define FORK_READERS 3 // count of readers to fork
#define FORK_WRITERS 2 // of writers
#define MAX_PROC FORK_READERS + FORK_WRITERS


#define SEM__COUNT 3
#define SEM_READING 0
#define SEM_WRITING 1
#define SEM_QDWRITERS 2



key_t shm_key = 1234; // key to get/init shm
int shm_flg = 0666; // flags of access to shm
int shm_size = sizeof(int); // size of shm block
int shm_id = 0; // id of acquired shm

// returns offset to block in shared-heap memory
char* GetSHM()
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




sembuf opStartReading[] = {
			{SEM_QDWRITERS, 0, 0},
			{SEM_WRITING, 0, 0},
			{SEM_READING, 1, 0}
		};		
sembuf opStopReading[]  = {
			{SEM_READING, -1, 0}
		};
		
/*sembuf opPlanWriting[] = {
		};*/
sembuf opStartWriting[] = {
			{SEM_QDWRITERS, 1, 0},
			{SEM_READING, 0, 0},
			{SEM_WRITING, -1, 0},
			{SEM_QDWRITERS, -1, 0},
		};
sembuf opStopWriting[]  = {
			{SEM_WRITING, 1, 0}
		};

double sleepR = 0.0;
double sleepW = 0.0; // time each process sleeps after doing operation

void debug_puts (const char* s)
{
	#ifdef DEBUG
		printf("%d : %s\n", getpid(), s );
	#endif
}

key_t sem_key = 5678; // key to get/init semaphore
int sem_flg = 0666; // flags of access to semaphore
int sem_cnt = SEM__COUNT; // count of semaphores in set
int sem_id = -1; // id of acquired semaphore set

// acquires semaphore set
int GetSem();

// initializes semaphore set' values and shared memory items
void Init();

// enables sleep for reals instead of integers
void sleep(double period)
{
	time_t s = (time_t)period;
	double ns = (period - s) * 1e9;
	timespec t = { s, (long)ns };
	//printf( "waitnig: %lf;  %d + %lf    /    %d + %d\n", period, s, ns, t.tv_sec, t.tv_nsec );
	nanosleep( &t, NULL );
}


#define OPCOUNT(o) sizeof(o)/sizeof(sembuf)
inline void StartRead()
{
	semop( sem_id, opStartReading, OPCOUNT(opStartReading) );
	printf( "  r %d: start\n", getpid() );
}
inline void StopRead()
{
	semop( sem_id, opStopReading, OPCOUNT(opStopReading) );
	printf( "  r %d: stop\n", getpid() );
}
inline void StartWrite()
{
	semop( sem_id, opStartWriting, OPCOUNT(opStartWriting) );
	printf( " w %d: start\n", getpid() );
	
}
inline void StopWrite()
{
	semop( sem_id, opStopWriting, OPCOUNT(opStopWriting) );
	printf( " w %d: stop\n", getpid() );
}


void Read(double tm)
{
	sem_id = GetSem();
	int* shm = (int*)GetSHM();
	while ( 1 )
	{
		StartRead();
		printf( "read var = %d\n", *shm );
		StopRead();
		sleep( tm );
	}
}


void Write(double tm)
{
	sem_id = GetSem();
	int* shm = (int*)GetSHM();
	while ( 1 )
	{
		StartWrite();
		(*shm)++;
		printf( "var is now %d\n", *shm );
		StopWrite();
		sleep( tm );
	}
}


int main(int argc, char* argv[])
{	
	int prnt = getpid();
	int chlds[MAX_PROC];
	Init();
	
	if ( argc > 1 )
	{
		sscanf( argv[1], "%lf", &sleepR );
		sscanf( argv[2], "%lf", &sleepW );
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
		if ( i < FORK_READERS )
			printf( " * main: startnig %d reader\n", chlds[i] );
		else
			printf( " * main: starting %d writer\n", chlds[i] );
	}
	
	// starting mainloops in children procs
	for (int i=0; i<MAX_PROC; i++)
	{
		if ( chlds[i] != 0 ) // skipping the cycle in main program and incorrect children
			continue;
		
		double r = rand() % 20; r /= 10.0;
		
		if ( i < FORK_READERS )
		{
			Read( r + sleepR );
			return 0;
		}
		else
		{
			Write( r + sleepW );
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


void Init()
{
	sem_id = semget( sem_key, sem_cnt, sem_flg | IPC_CREAT );
	
	if ( sem_id == -1 )
	{
		perror( "semget (init)" );
		exit(1);
	}

	unsigned short vals[] = { 0, 1, 0 };
	int c = semctl( sem_id, 0, SETALL, vals );
	
	if ( c == -1 )
	{
		perror( "init semctl" );
		exit( 7 );
	}
	
	int* shm = (int*)GetSHM();
	memset( shm, 0, sizeof(int) );

}
