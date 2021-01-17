#include <stdio.h>
#include <stdlib.h> //rand
#include <conio.h>
#include <Windows.h>

#define MAX_READERS 6

#define WAIT_IN_THREAD

HANDLE evCanRead = NULL; //хэндлы событий, сигнализирующих о завершении действия
HANDLE evCanWrite = NULL;

HANDLE thReaders[MAX_READERS] = {0}; //хэндлы потоков
HANDLE thWriter = NULL;

unsigned readersActive = 0;

unsigned readersWaiting = 0; //определение, чья очередь
unsigned writersWaiting = 0;

bool IsWriting = false;



void StartRead()
{
	if ( IsWriting || writersWaiting > 0 )
	{
		InterlockedIncrement( &readersWaiting );
		printf_s( "reader %d is waiting\n", GetCurrentThreadId() );
		WaitForSingleObject( evCanRead, INFINITE );
	}

	printf_s( "reader %d is reading\n", GetCurrentThreadId() );
	InterlockedIncrement( &readersActive );
	InterlockedDecrement( &readersWaiting );
	SetEvent( evCanRead );
}

void StopRead()
{
	printf_s( "reader %d finished\n", GetCurrentThreadId(), writersWaiting );
	InterlockedDecrement( &readersActive );

	if ( readersActive == 0 )
	{
		ResetEvent( evCanRead );
		SetEvent( evCanWrite );
	}

	WaitForSingleObject( evCanWrite, INFINITE ); //чтобы читатель i не начинал читать заново сразу после своего завершения
}

void StartWrite()
{
	if ( readersActive > 0 || readersWaiting > 0 )
	{
		InterlockedIncrement( &writersWaiting );
		printf_s( "writer is waiting\n" );
		WaitForSingleObject( evCanWrite, INFINITE );
	}

	if ( writersWaiting ) //чтобы на старте не декрементилось в -1
		InterlockedDecrement( &writersWaiting );
	IsWriting = true;
	printf_s( "writer is writing\n" );
}

void StopWrite()
{
	IsWriting = false;
	printf_s( "writer finished\n");

	ResetEvent( evCanWrite );
	ResetEvent( evCanRead );

	if ( readersWaiting > 0 )
		SetEvent( evCanRead );
	else
		SetEvent( evCanWrite );
}



DWORD WINAPI Reader_func(LPVOID param)
{
	while (1)
	{
		StartRead();

#ifdef WAIT_IN_THREAD
		Sleep( 100 * ((rand() % 7)+1) );
#endif

		StopRead();
	}
	
	return NULL;
}

DWORD WINAPI Writer_func(LPVOID param)
{
	while (1)
	{
		StartWrite();

#ifdef WAIT_IN_THREAD
		Sleep( 200 * ((rand() % 4)+1) );
#endif

		StopWrite();
	}
	return NULL;
}

void Init()
{
	thWriter = CreateThread( NULL, 0, Writer_func, NULL, NULL, NULL );
	for (int i = 0; i < MAX_READERS; i++)
	{
		thReaders[i] = CreateThread( NULL, 0, Reader_func, NULL, NULL, NULL );
	}
	evCanRead = CreateEvent( NULL, true, false, "CanRead" );
	evCanWrite = CreateEvent( NULL, true, false, "CanWrite" );
}

int main()
{
	Init();

	SetEvent( evCanWrite ); //изначально запускается поток писателя

#ifdef WAIT_IN_THREAD
	_getch();
#else
	for (int i=0; i < 10000000; i++)
		;
#endif

	SuspendThread( thWriter );
	for (int i=0; i<MAX_READERS; i++)
		SuspendThread( thReaders[i] );
	
	printf_s( "main: press any key to exit\n" );
	_getch();
	return 0;
}