
#include "Thread.h"

pthread_t*	threads;
int*		threadIDs;
int			maxThreads = 2, activeThreads = 0;
int 		POWER = 1;
int			total = 0;
int*		slots;
int 		size = 1000;

pthread_t tManT;
pthread_mutex_t mainMutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
	int i = 0;

	threads = (pthread_t*) malloc(sizeof(pthread_t) * maxThreads);
	threadIDs = (int*) malloc(sizeof(int) * maxThreads);
	slots = (int*) malloc(sizeof(int) * size);

	for (; i < size; ++i)
		slots[i] = -1;

	pthread_create(&tManT, NULL, (void*) &threadManager, NULL);

	i = 0;
	for (; i < size; ++i) {
		pthread_mutex_lock(&mainMutex);
		int id = getSlot();
		while (id == -1) {
			usleep(10000);
			id = getSlot();
		}

		total++;
		threadIDs[id] = pthread_create(&(threads[id]), NULL,(void*) &printAndKill, i);
		slots[i] = id;
		activeThreads++;
		pthread_mutex_unlock(&mainMutex);
		usleep(1000);
	}

	for (i = 0; i < size; ++i)
		if (slots[i] != -1) {
			pthread_mutex_lock(&mainMutex);
			pthread_join(threads[slots[i]], NULL);
			pthread_mutex_unlock(&mainMutex);
		}

	sleep(2);

	POWER = 0;
	pthread_join(tManT, NULL);
	return 0;
}

void printAndKill(int id) {
	int j = 0;
	for (; j < 100000000; ++j);
	activeThreads--;
	slots[id] = -1;
	printf("Thread %d Ended\n", id);
}

int getSlot() {
	if (activeThreads >= MAX_THREADS)
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
			threads = (pthread_t*) realloc(threads, sizeof(pthread_t) * maxThreads);
			threadIDs = (int*) realloc(threadIDs, sizeof(int) * maxThreads);
			pthread_mutex_unlock(&mainMutex);
		}
		if (activeThreads + 5 < maxThreads / 4 && maxThreads > 4) {
			pthread_mutex_lock(&mainMutex);
			maxThreads /= 2;

			ArrangeSlots();

			threads = (pthread_t*) realloc(threads, sizeof(pthread_t) * maxThreads);
			threadIDs = (int*) realloc(threadIDs, sizeof(int) * maxThreads);
			pthread_mutex_unlock(&mainMutex);
		}
	}
	pthread_kill(tManT, SIGINT);
}
