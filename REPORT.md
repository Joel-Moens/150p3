# ECS 150: Project 3 "User-Level Thread Library (Part 2)"
### Partners: Joel Moens, Akshay Kumar
#### Due Date: Friday, February 23rd, 2018

-------------------------------------------------------------------------------

## Program Implementation
### Phase 1: Semaphore API
We implemented the following functions from sem.h:

* sem_t sem_create(size_t count);
* int sem_destroy(sem_t sem);
* int sem_down(sem_t sem);
* int sem_up(sem_t sem);

We defined our semaphore as a struct which contains: a queue and a count
variable. Our queue is used to keep hold of TIDs, which are found within
threads. We decrement our count whenever threads ask for a resource from our 
semaphore and increment our count whenever threads give a resource back to our 
semaphore. If a thread asks for a resource but the count of our semaphore is 
at 0, then we enqueue the thread's TID and call upon it whenever the semaphore
has a resource to distribute. In order to prevent threads from acting while 
they are waiting for a resource, we block them before enqueing them. We 
unblock them after dequeuing them so they can resume execution. Since we don't
want multiple active threads running during this sensitive process, we create a
critical section which guarantees mutual exclusivity. This only allows one 
thread to run the desired section of code. In order to make critical sections,
block and unblock threads, we use the following functions from thread.h:

* int thread_block(void);
* int thread_unblock(pthread_t tid);
* void enter_critical_section(void);
* void exit_critical_section(void);


### Phase 2: TPS API
#### Phase 2.1: Unprotected TPS With Naive Cloning
We implemented the following functions from tps.h:

* int tps_init(int segv);
* int tps_create(void);
* int tps_destroy(void);
* int tps_read(size_t offset, size_t length, char \*buffer);
* int tps_write(size_t offset, size_t length, char \*buffer);
* int tps_clone(pthread_t tid);

We also defined two data structures and made two helper functions:

* struct tps; 
* queue_t tQueue;
* static int find_item(queue_t queue, void \*data, void \*arg);
* struct tps * tps_malloc();

For Phase 2.1 tps_init() just simply creates tQueue for future use by the other
tps functions. tps_malloc() allocates mem for a tps for the current running
thread. tQueue holds any tps created by tps_create() or by tps_clone().
With the use of queue_iterate and a helper function find_item, we were able to 
search the queue immediately after every tps method is called in order to
determine the current tps to perform operations on. The current tps is simply 
determined with a pthread_self() and checked against every thread id in the
queue. At this point in phase 2 the tps struct was simply a void * to the
mapped memory as well as a pthread_t pointing to the tps thread id. We map the
memory for a new tps inside tps_create() and tps_clone(); We give map read/
write permissions, set the MAP_ANONYMOUS and MAP_SHARED flags, with a file 
descriptor of -1 (for MAP_ANON), and a size of TPS_SIZE. The tps_write() and
tps_read() method both cast the buffer to a void * then offset the mapped mem
to the correct address and either memcpy the buffer into memory or memory into
buffer. tps_clone at this point simply creates a new tps, maps new memory, and 
copies the parent memory into the new tps memory, this is later changed in
2.3. tps_clone uses find_item differently because it passes the given tid 
argument into find_item as an argument. tps_destroy just iterates through the
queue, munmaps current thread tps removes it from the queue.
Also, for every tps function except tps_init() we enter a critical section to 
ensure mutual exclusion, then we exit the critical sections right before we 
return from the method.

#### Phase 2.2: Protected TPS
We modified tps_init, added a fault handler function, and added a queue func:

* static void segv_handler(int sig, siginfo_t \*si, void \*context);
* static int find_item_by_address(queue_t queue, void \*data, void \*arg);

In order to ensure that all TPS are protected from reading and writing outside
of tps_read and tps_write, we had to create a sigaction in tps_init() to add
more functionality to the SIGBUS and SIGSEGV signals. Any time these signals 
are called segv_handler is called with memory address trying to be accessed.
segv_handler then iterates through the queue searching by memory address. If
found, then we know this was a TPS protection error because the something tried
to alter tps memory. We also had to alter our read/write permissions when we 
mmap for memory pages. Before, we mapped with read and write permissions, now
we give the memory no permissions, and only inside tps_read and tps_write do we
give the memory permission to be read or written to respectively. This is done
with the function mprotect.


#### Phase 2.3: Copy-On-Write Cloning
We added a page struct, modified tps_clone, tps_write, and tps_destroy

* typedef struct page

Phase 2.3 required that we change how we clone a new tps, so that the memory
given by the parent to the clone is simply a pointer to the same memory. This,
however required us to create a further abstraction of the memory address into
a struct named page. The page struct contains a void * and a counter which 
retains the number of tps associated with this memory. Our tps struct now holds
a page* instead of a void* address. In tps_clone we give the clone the same
mem_page as the parent. This way whenever the clone reads, it reads from the 
same memory address as the parent. This means that tps_clone no longer mmaps 
the clone's memory page, instead clone creates a new memory page and mmaps in
the first tps_write call the clone receives. Before mapping to new memory,
however, the clone must reduce the counter on the shared memory page.
Initially I thought only tps_write and tps_clone needed to be altered, but 
tps_destroy also needed modification. For if a memory page was shared by two
or more tps, then we shouldn't be munmapping the page, instead we decrement
the counter and we set the mem_page pointer to NULL. If the mem_page counter
is 1, though, then we munmap. Phase 2.3 also required that we add read/write 
permissions with mprotect if a shared memory page is found. Parent must be able
to write, and clone must be able to read into the new mmaped memory.


## Additional Code
### Makefile Development
Within the ~/test/Makefile, we only had to modify the programs header to
include our tester file and to wrap the mmap function for testing. As for the
~/libuthread/Makefile, we needed to make use of the queue.o and thread.o 
executables in order to create executables for sem.c and tps.c. We generated 
our static library libuthread.a in the same manner as we did for the last 
program. We lastly provided the option to clean our sem.o, tps.o, and 
libuthread.a files. We could not clean all .o files because we needed to hold 
onto queue.o and thread.o, which are given. 


### Testing File
#### tester.c
Our own testing file was a more robust adaptation of the tps.c file that was 
given in the test directory. In this tester we added two more threads and two 
more semaphores to test TPS protection and copy on write functionality. If the 
tester is made and run as given: the program will create a tps in thread1, grab
the memory address mmap gives from the wrapper function, and try to alter it. 
This will cause a TPS Protection error, demonstrating that phase 2.2 has been
completed. Once we see 2.2 works, we then comment out the first few lines of 
thread 1 (Comments state where to comment), and we can test phase 2 completely.
We now test writing given messages into memory (with and without offsets),
reading memory into buffers (with and without offsets), and asserting that 
there is no corruption or mismanagment of data from tps_write to tps_read.
We also create and write to a tps in thread4, clone this tps in thread3, 
destroy the tps in thread4, and assert on return to thread3 that the memory 
still remains in thread3's tps. We then proceed to clone thread3 in thread2
and clone thread2 in thread3, asserting that data is still stable and 
uncorrupt after these operations by reading into the buffer and asserting.
We also test writing and reading a limited (not TPS_SIZE) amount of memory,
which all works. We don't show that cloned threads do not have a unique mmap
until they are written into, but this can be easily seen in our actual tps.c
code in tps_write and tps_clone. 
