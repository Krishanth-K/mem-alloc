// AI GENERATED
// ==============================================================================================================================

#include "mem.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 4
#define NUM_ITERATIONS 1000

void *thread_func(void *arg)
{
	int id = *(int *)arg;
	printf("Thread %d starting...\n", id);

	for (int i = 0; i < NUM_ITERATIONS; i++)
	{
		// Allocate a random size
		size_t size = (rand() % 128) + 16;
		void *ptr = _malloc(size);

		if (!ptr)
		{
			fprintf(stderr, "Thread %d: malloc failed\n", id);
			continue;
		}

		// Write to it (corruption check)
		*(int *)ptr = id;

		// Small delay to increase chance of race condition
		if (i % 100 == 0)
			usleep(1);

		// Verify content
		if (*(int *)ptr != id)
		{
			fprintf(
			    stderr,
			    "Thread %d: Data corruption detected! Expected %d, got %d\n",
			    id, id, *(int *)ptr);
		}

		_free(ptr);
	}

	printf("Thread %d finished.\n", id);
	return NULL;
}

int main()
{
	pthread_t threads[NUM_THREADS];
	int thread_ids[NUM_THREADS];

	printf("Starting multi-threaded stress test with %d threads...\n",
	       NUM_THREADS);
	printf(
	    "Expect crashes (segfaults) or corruption due to lack of mutexes.\n");

	for (int i = 0; i < NUM_THREADS; i++)
	{
		thread_ids[i] = i;
		if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0)
		{
			perror("pthread_create failed");
			return 1;
		}
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}

	printf("Test finished (surprisingly without crashing if you see this).\n");
	return 0;
}
