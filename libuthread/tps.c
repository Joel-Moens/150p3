#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

#define REMOVE 1
#define KEEP 0

/* TODO: Phase 2 */

struct page {
	void * address;
	int counter;
};
struct tps {
	pthread_t id;
	struct page * mem_page;
};
queue_t tQueue;

static int find_item(queue_t queue, void *data, void *arg)
{	
	pthread_t self;
	if(arg == NULL)
	{
		self = pthread_self();	
	} // not given a tid so tid is current thread
	else
	{
		self = ((struct tps*)arg)->id;
	} // given a tid so use it

	pthread_t curr_id = ((struct tps*)data)->id; // curr_id is current thread in queue_iterate
	//printf("Inside find item %lu against %lu \n", self, curr_id);
	if(curr_id == self) 
	{
		//printf("Found item \n");
		return 1;
	}
	//printf("Didn't find item \n");
    return 0;
}
static int find_item_by_address(queue_t queue, void *data, void *arg)
{	
	void* self = arg;
	void* curr_address = ((struct tps*)data)->mem_page->address; // curr_id is current thread in queue_iterate
	//printf("Inside find item %lu against %lu \n", self, curr_id);
	if(curr_address == self) 
	{
		//printf("Found item \n");
		return 1;
	}
	//printf("Didn't find item \n");
    return 0;
}
struct tps * tps_malloc()
{
	struct tps * tps_space = (struct tps *) malloc(sizeof(struct tps));
	tps_space->id = pthread_self();
	tps_space->mem_page= NULL;
	return tps_space;
} // allocate memory for a new tps, tid is cur running thread

static void segv_handler(int sig, siginfo_t *si, void *context)
{
    /*
     * Get the address corresponding to the beginning of the page where the
     * fault occurred
     */
    void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));

    /*
     * Iterate through all the TPS areas and find if p_fault matches one of them
     */
    void * data = NULL;
	queue_iterate(tQueue, find_item_by_address, p_fault, &data);
	/* There is a match */
    if (((struct tps*)data)->mem_page->address == p_fault)
    {
        fprintf(stderr, "TPS protection error!\n");
        /* Printf the following error message */	
    }
    /* In any case, restore the default signal handlers */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    /* And transmit the signal again in order to cause the program to crash */
    raise(sig);
}

int tps_init(int segv)
{
	/* TODO: Phase 2 */
	//AKSHAY you will have to add a lot more in this for phase 2.2
	tQueue = queue_create();

	// checks for signals of type SIGSEGV and SIGBUS
    if (segv) {
        struct sigaction sa;

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = segv_handler;
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGSEGV, &sa, NULL);
    }

	return 0;
}

int tps_create(void)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	void * data = NULL;
	queue_iterate(tQueue, find_item, NULL, &data);
	if(data != NULL)
	{
		//printf("ERROR: TPS ALREADY CREATED \n");
		exit_critical_section();
		return -1;
	} // Tps was found in the queue before being made

	// If TPS not found make a new TPS and page and map for it
	struct tps * newT = tps_malloc();
	newT->mem_page = (struct page*) malloc(sizeof(struct page));
	newT->mem_page->address = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if(newT->mem_page->address == MAP_FAILED)
	{
		printf("Error Trying to Map \n");
		return -1;
	} // Map failed
	newT->mem_page->counter = 1; //Begin the page counter
	queue_enqueue(tQueue, newT);
	//printf("Queue size: %d\n", queue_length(tQueue));
	exit_critical_section();
	return 0;

}

int tps_destroy(void)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	struct tps * found;
	void * data = NULL;
	queue_iterate(tQueue, find_item, NULL, &data);
	if(data != NULL)
	{
		found = (struct tps*)data;
		if(found->mem_page->counter > 1)
		{
			found->mem_page->counter--;
			found->mem_page = NULL;
		}
		else
		{
			munmap(found->mem_page->address, TPS_SIZE);
		}
		queue_delete(tQueue, data);
		exit_critical_section();
		return 0;
	} //TPS found will delete
	else
	{
		//printf("ERROR: TPS NOT FOUND \n");
		exit_critical_section();
		return -1;
	} //TPS not found cannot delete
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	struct tps * found;
	//printf("Trying to read from a tps for %lu \n", pthread_self());
	void * data = NULL;
	queue_iterate(tQueue, find_item, NULL, &data);
	if(data != NULL)
	{
		found = ((struct tps *)data);
		mprotect(found->mem_page->address,TPS_SIZE,PROT_READ);
		void * mem_loc = found->mem_page->address;
		//printf("%s\n", (char*)mem_loc);
		mem_loc += offset;
		memcpy((void*)buffer, mem_loc, length);
		mprotect(found->mem_page->address,TPS_SIZE,PROT_NONE);
		exit_critical_section();
		return 0;
	} // found tps copy the memory from page to buffer
	else
	{
		printf("ERROR: TPS NOT FOUND \n");
		exit_critical_section();
		return -1; 	
	} // tps never found in the queue
	
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	struct tps * found;
	//printf("Trying to write to a tps for %lu \n", pthread_self());
	void * data = NULL;
	queue_iterate(tQueue, find_item, NULL, &data);
	if(data != NULL)
	{
		found = ((struct tps*)data);
		if(found->mem_page->counter > 1)
		{
			// Shared page, keep prev reference to copy into new page
			struct page * prev = found->mem_page;
			prev->counter--;
			struct page * new = (struct page*) malloc(sizeof(struct page));
			new->address = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
			if(new->address == MAP_FAILED)
			{
				printf("Error Trying to Map \n");
				return -1;
			} //Failed to map memory		
			new->counter = 1;
			mprotect(new->address,TPS_SIZE,PROT_WRITE);
			mprotect(prev->address,TPS_SIZE,PROT_READ);
			memcpy(new->address, prev->address, TPS_SIZE);
			found->mem_page = new;
			mprotect(prev->address,TPS_SIZE,PROT_NONE);
			mprotect(new->address,TPS_SIZE,PROT_NONE);
		} // Memory Page is being shared need to make a new page for this TPS   
		mprotect(found->mem_page->address,TPS_SIZE,PROT_WRITE);
		void * mem_loc = found->mem_page->address;
		mem_loc += offset;
		memcpy(mem_loc, (void*)buffer, length - offset);
		mprotect(found->mem_page->address,TPS_SIZE,PROT_NONE);
		exit_critical_section();
		return 0;
	} // Found this tps copy buffer into page
	else
	{
		//printf("ERROR: TPS NOT FOUND \n");
		exit_critical_section();
		return -1;
	} // tps never found in the queue
	
}

int tps_clone(pthread_t tid)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	struct tps * found;
	//printf("Trying to clone TPS with tid: %lu \n", tid);
	void * data = NULL;
	queue_iterate(tQueue, find_item, NULL, &data);
	if(data != NULL)
	{
		printf("ERROR: CURRENT TID TPS FOUND \n");
		exit_critical_section();
		return -1;
	} //This thread already has a TPS
	queue_iterate(tQueue, find_item, (void*)tid, &data);
	if(data != NULL)
	{
		//printf("FOUND GIVEN TPS TO CLONE \n");
		found = ((struct tps *)data);
		struct tps * clone = tps_malloc();
		clone->mem_page = found->mem_page;
		found->mem_page->counter++;
		queue_enqueue(tQueue, clone);
		exit_critical_section();
		return 0;
			
	} //Given thread has a TPS so copy
	
	//Given thread doesn't have a TPS so error
	exit_critical_section();
	return -1;
}

