
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

void init() {
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

	i = 0;
	for (; i < size; ++i) {
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
		threadIDs[id] = pthread_create(&(threads[id]), NULL,(void*) &printAndKill, j);
		slots[j] = id;
		pthread_mutex_unlock(&mainMutex);
		usleep(10000);
	}

	for (i = 0; i < maxThreads; ++i) {
		if (threadStates[i] != -1) {
			pthread_mutex_lock(&mainMutex);
			pthread_join(threads[i], NULL);
			pthread_mutex_unlock(&mainMutex);
		}
	}

	sleep(2);

	POWER = 0;
	pthread_join(tManT, NULL);
	return 0;
}

void printAndKill(int id) {
	int j = 0;
	for (; j < 100000000; ++j);
	freeThread(id);
	activeThreads--;
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
	printf("Total:%d\tA:%d\tM:%d\n", total, activeThreads, maxThreads);
}
