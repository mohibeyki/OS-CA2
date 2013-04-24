
#include "Thread.h"

pthread_t*	threads;
int*		threadIDs;
int*		threadStates;

int			maxThreads = 4, activeThreads = 0;
int 		POWER = 1;
int			total = 0;
int*		slots;
int 		size = 1000;

pthread_t 	tManT;
pthread_mutex_t 	mainMutex = PTHREAD_MUTEX_INITIALIZER;

void initializeThreadManager() {
	int i = 0;
	threads = (pthread_t*) malloc(sizeof(pthread_t) * maxThreads);
	threadIDs = (int*) malloc(sizeof(int) * maxThreads);
	threadStates = (int*) malloc(sizeof(int) * maxThreads);

	for (; i < maxThreads; ++i)
		threadStates[i] = -1;
	slots = (int*) malloc(sizeof(int) * GLOBAL_MAX_THREADS_ALLOWED);

	for (i = 0; i < GLOBAL_MAX_THREADS_ALLOWED; ++i)
		slots[i] = -1;

	pthread_create(&tManT, NULL, (void*) &threadManager, NULL);
}

void killThreadManager() {
	POWER = 0;
	pthread_join(tManT, NULL);
}

void runThread(void* func) {
	int j = getSlot();
	while (j == -1) {
		usleep(10000);
		j = getSlot();
	}
	int id = getFreeThreadID();
	while (id == -1) {
		usleep(10000);
		id = getFreeThreadID();
	}
	total++;
	activeThreads++;
	pthread_mutex_lock(&mainMutex);
	threadIDs[id] = pthread_create(&(threads[id]), NULL, func, j);
	threadStates[id] = 1;
	slots[j] = id;
	pthread_mutex_unlock(&mainMutex);
}

void joinThreads() {
	while (activeThreads > 0) {
		int i = 0;
		for (; i < maxThreads; ++i)
			if (threadStates[i] != -1)
				pthread_join(threads[i], NULL);
	}
}

int getSlot() {
	if (activeThreads >= GLOBAL_MAX_ACT_THREADS)
		return -1;
	int i = 0;
	for (; i < maxThreads; ++i)
		if (slots[i] == -1)
			return i;
	return -1;
}

void ArrangeSlots() {
	int i = maxThreads * 2 - 1;

	for (; i >= maxThreads; --i) {
		if (slots[i] > maxThreads) {
			int id = getSlot();
			threads[id] = threads[slots[i]];
			threadIDs[id] = threadIDs[slots[i]];
			slots[i] = id;
		}
	}
}

void threadManager() {
	while (POWER) {
		if (activeThreads + 5 > maxThreads) {
			pthread_mutex_lock(&mainMutex);
			maxThreads *= 2;
			threads = (pthread_t *) realloc(threads, sizeof(pthread_t) * maxThreads);
			threadIDs = (int *) realloc(threadIDs, sizeof(int) * maxThreads);
			threadStates = (int *) realloc(threadStates, sizeof(int) * maxThreads);
			int i = maxThreads / 2;
			for (; i < maxThreads; ++i)
				threadStates[i] = -1;
			pthread_mutex_unlock(&mainMutex);
		}
		if (activeThreads + 5 < maxThreads / 4 && maxThreads > 4) {
			pthread_mutex_lock(&mainMutex);
			maxThreads /= 2;
			ArrangeSlots();
			threads = (pthread_t *) realloc(threads, sizeof(pthread_t) * maxThreads);
			threadIDs = (int *) realloc(threadIDs, sizeof(int) * maxThreads);
			threadStates = (int *) realloc(threadStates, sizeof(int) * maxThreads);
			pthread_mutex_unlock(&mainMutex);
		}
		usleep(10000);
	}
	pthread_kill(tManT, SIGINT);
}

int getFreeThreadID() {
	int i = 0;
	for (; i < maxThreads; ++i)
		if (threadStates[i] == -1)
			return i;
	return -1;
}

void freeThread(int id) {
	threadStates[slots[id]] = -1;
	slots[id] = -1;
	activeThreads--;
	printf("A Connection has been closed\nTotal Threads:%d\tA:%d\tM:%d\n", total, activeThreads, maxThreads);
}
