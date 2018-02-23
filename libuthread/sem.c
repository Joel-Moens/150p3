#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
	/* TODO: Phase 1 */
	queue_t Q;
	size_t count;
};

sem_t sem_create(size_t count)
{
	/* TODO: Phase 1 */
	// sem is initialized to null
	sem_t sem = (sem_t) malloc(sizeof(*(sem_t){NULL}));
	// queue is initialized to null, count is given
	sem->Q = queue_create();
	sem->count = count;
	return sem;
}

int sem_destroy(sem_t sem)
{
	/* TODO: Phase 1 */
	// sem is null or threads are being blocked
	if (!sem || queue_length(sem->Q) != 0)
		return -1;
	queue_destroy(sem->Q);
	free(sem);
	sem = NULL;
	return 0;
}

int sem_down(sem_t sem)
{
	/* TODO: Phase 1 */
	// grabbing a resource
	// sem is null
	if (!sem)
		return -1;
	enter_critical_section();
	// thread needs to be blocked
	if (sem->count == 0) {
		// creating pointer to thread for enqueuing
		pthread_t tid = pthread_self();
		queue_enqueue(sem->Q,&tid);
		thread_block();
	}
	else 
		sem->count--;
	exit_critical_section();
	return 0;
}

int sem_up(sem_t sem)
{
	/* TODO: Phase 1 */
	// releasing a resource
	// sem is null
	if (!sem)
		return -1;
	enter_critical_section();
	// first blocked thread can grab a resource
	if (sem->count == 0 && queue_length(sem->Q) != 0) {
		// creating pointer to pointer to hold dequeued thread
		void *data;
		queue_dequeue(sem->Q,&data);
		thread_unblock(*((pthread_t *) data));
	}
	else 
		sem->count++;
	exit_critical_section();
	return 0;
}