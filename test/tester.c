#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>
/*
THIS IS THE TESTER WE CREATED TO TEST PHASE 2.2 and 2.3
Test for Phase2.2 is in thread1, comment out beginning section to test a correct tester for 2.3 cloning
*/
char msg1[TPS_SIZE] = "This is a clone\n";
char msg2[TPS_SIZE] = "This isn't a clone\n";
char msg3[TPS_SIZE] = "This was a clone\n";
char msg4[TPS_SIZE-1] = "This wasn't a clone \n";
char msg5[TPS_SIZE] = "m I a clone?\n";
char msg6[TPS_SIZE] = "I am not a clone\n";
char msg7[TPS_SIZE] = "Am I not a clone\n";

void *latest_mmap_addr; // make the address returned by mmap accessible

void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
    return latest_mmap_addr;
}

sem_t sem1, sem2, sem3, sem4;
void* thread4(void* arg)
{
	char *buffer = malloc(TPS_SIZE);
	tps_create();
	tps_write(0, TPS_SIZE, msg3);
	printf("thread4: Wrote msg3 returning to thread3 to clone\n");
	sem_up(sem3);
	sem_down(sem4);
	tps_read(0, TPS_SIZE, buffer);
	tps_destroy();
	printf("thread4: Destroyed tps returning to clone\n");
	sem_up(sem3);
	return NULL;
} //thread4 creates a tps writes message 3 concedes to thread3 which clones thread4, then destroy thread4
void* thread3(void* arg)
{
	pthread_t tid4;
	char *buffer = malloc(TPS_SIZE);
	/* Create thread 4 and get blocked */
	pthread_create(&tid4, NULL, thread4, NULL);
	sem_down(sem3);
	tps_clone(tid4);
	sem_up(sem4);
	sem_down(sem3);
	/* Read from TPS and make sure it contains the cloned message */
	memset(buffer, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buffer);
	assert(!memcmp(msg3, buffer, TPS_SIZE));
	printf("thread3: read msg3 OK!\nBuffer: %s \n",buffer);
	/* 
	Unlock thread 2 and block thread 3 
	*/
	sem_up(sem2);
	printf("unlocking thread2\n");
	sem_down(sem3);
	/* Come back from thread 2 */
	printf("back in thread3\n");
	tps_read(0, TPS_SIZE, buffer);
	assert(!memcmp(msg3, buffer, TPS_SIZE));
	printf("thread3: returning from being cloned twice read OK!\nBuffer: %s \n",buffer);
	/*Write in msg6 and change it to equal msg7*/
	tps_write(0, TPS_SIZE, msg6);
	//"I Am" --> "Am I"
	buffer[0] = 'A';
	buffer[1] = 'm';
	buffer[2] = ' ';
	buffer[3] = 'I';
	tps_write(0, 4, buffer);
	tps_read(0, TPS_SIZE, buffer);
	/* Assert that msg2 modified == msg1  */
	assert(!memcmp(msg7, buffer, TPS_SIZE));
	printf("thread3: read and modified msg6-->msg7 read OK!\nBuffer: %s \n",buffer);

	/* Transfer CPU to thread 1 and get blocked */
	sem_up(sem2);
	sem_down(sem3);

	/* Destroy TPS and quit */
	printf("Destroying thread 3 \n");
	tps_destroy();
	return NULL;
}

void* thread2(void* arg)
{
	pthread_t tid3;
	char *buffer = malloc(TPS_SIZE);

	/* Create thread 2 and get blocked */
	pthread_create(&tid3, NULL, thread3, NULL);

	/* Create TPS and initialize with *msg1 */
	tps_create();
	tps_write(1, TPS_SIZE-1, msg4);
	/* Read from TPS and make sure it contains the message */
	memset(buffer, 0, TPS_SIZE);
	tps_read(1, TPS_SIZE, buffer);
	assert(!memcmp(msg4, buffer, TPS_SIZE-1));
	//This wasn't a clone
	printf("thread2: read OK!\nBuffer: %s \n",buffer);

	/* 
	Block for thread 3 
	*/
	sem_down(sem2);
	printf("back in thread2\n");
	tps_destroy();
	tps_clone(tid3);
	sem_up(sem1);
	printf("unlocking thread1\n");
	sem_down(sem2);

	/* When we're back, read TPS and make sure it sill contains the original */
	memset(buffer, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buffer);
	assert(!memcmp(msg3, buffer, TPS_SIZE));
	//This wasn't a clone
	printf("thread2: clone read OK!\nBuffer: %s \n",buffer);

	/* Transfer CPU to thread 1 and get blocked */
	sem_up(sem3);
	sem_down(sem2);
	sem_up(sem3);
	sem_up(sem1);
	sem_down(sem2);

	/* Destroy TPS and quit */
	printf("Destroying thread 2\n");
	tps_destroy();
	return NULL;
}

void* thread1(void* arg)
{

	/* COMMENT THIS CODE OUT TO TEST ALL THREADS THIS IS JUST TO TEST TPS PROTECTION */
	char *tps_addr;
	/* Create TPS */
	tps_create();
	//Get TPS page address as allocated via mmap() 
	tps_addr = latest_mmap_addr;
	/* Cause an intentional TPS protection error */
	tps_addr[0] = '\0';
	/* COMMENT THIS CODE OUT TO TEST ALL THREADS THIS IS JUST TO TEST TPS PROTECTION */

	pthread_t tid2;
	char *buffer = malloc(TPS_SIZE);

	/* Create thread 2 and get blocked */
	pthread_create(&tid2, NULL, thread2, NULL);
	sem_down(sem1);
	printf("Cloning thread 2's tps, which is a clone of thread 3\n");
	tps_clone(tid2);
	/* Read the TPS and make sure it contains the original */
	memset(buffer, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buffer);
	assert(!memcmp(msg3, buffer, TPS_SIZE));
	printf("thread1: read clone OK!\nBuffer: %s \n",buffer);
	/* Modify TPS to cause a copy on write */
	tps_write(0, TPS_SIZE, msg1);
	printf("thread1: copied and wrote OK!\n");
	/* Transfer CPU to thread 2 and get blocked */
	sem_up(sem2);
	printf("unlocked thread2\n");
	sem_down(sem1);
	/* When we're back, make sure our modification is still there */
	memset(buffer, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buffer);
	assert(!memcmp(msg1, buffer,TPS_SIZE));
	printf("thread1: return thread2 and read OK!\nBuffer: %s \n",buffer);
	/* Now write to an offset, and read to an offset */
	tps_write(1, TPS_SIZE-1, msg5);
	printf("thread1: offset write worked\n");
	buffer[0] = 'A';
	tps_write(0, 1, buffer);
	printf("thread1: Buffer before offset read\nBuffer: %s \n",buffer);
	tps_read(0, TPS_SIZE, buffer);
	printf("thread1: normal read\nBuffer: %s \n",buffer);
	tps_read(5, TPS_SIZE-5, buffer);
	printf("thread1: offset read\nBuffer: %s \n",buffer);
	/* Transfer CPU to thread 2 */
	sem_up(sem2);

	/* Wait for thread2 to die, and quit */
	pthread_join(tid2, NULL);
	printf("Destroying myself...\nGoodbye\n");
	tps_destroy();
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t tid;

	/* Create two semaphores for thread synchro */
	sem1 = sem_create(0);
	sem2 = sem_create(0);
	sem3 = sem_create(0);
	sem4 = sem_create(0);

	/* Init TPS API */
	tps_init(1);

	/* Create thread 1 and wait */
	pthread_create(&tid, NULL, thread1, NULL);
	pthread_join(tid, NULL);

	/* Destroy resources and quit */
	sem_destroy(sem1);
	sem_destroy(sem2);
	sem_destroy(sem3);
	sem_destroy(sem4);
	return 0;
}
