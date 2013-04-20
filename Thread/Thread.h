
#pragma once

#include "../Common/Common.h"

#define MAX_THREADS 5

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
