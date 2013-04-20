#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define MAX_THREADS 80

extern	pthread_t*	threads;
extern	int*		threadIDs;
extern	int*		slots;
extern	int			maxThreads, activeThreads, total, size;
extern	int 		IS_ALIVE;

extern	pthread_t 	tManT;
extern	pthread_mutex_t 	mainMutex;

void printAndKill(int id);
int getSlot();
void ArrangeSlots();
void threadManager();
