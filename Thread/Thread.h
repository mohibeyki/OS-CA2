
#pragma once

#include "../Common/Common.h"

#define GLOBAL_MAX_ACT_THREADS		64
#define GLOBAL_MAX_THREADS_ALLOWED	128

extern	pthread_t*	threads;
extern	int*		threadIDs;
extern	int*		threadStates;
extern	int*		slots;
extern	int			maxThreads, activeThreads, total, size;
extern	int 		IS_ALIVE;

extern	pthread_t 	tManT;
extern	pthread_mutex_t 	mainMutex;

void	init();
void 	printAndKill(int id);
int 	getSlot();
void 	ArrangeSlots();
void 	threadManager();
int 	getFreeThreadID();
void 	freeThread(int id);
