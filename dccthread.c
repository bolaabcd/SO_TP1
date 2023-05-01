#include "dccthread.h"
#include <ucontext.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

// Maximum number of threads and bytes on the stack of each thread.
#define MAX_THREADS 1000
#define MAX_BYTES_PER_STACK 8192

// Structure with data for the thread
struct dccthread {
	const char* name;
	ucontext_t context;
	int non_empty;
	int executing;
};

// The first thread of the list will be the scheduler.
// Notice that threads[i].non_empty and threads[i].executing will be initialized to 0, as this is global initialization.
dccthread_t threads[MAX_THREADS];

// The stacks
char stacks[MAX_THREADS][MAX_BYTES_PER_STACK];

// IS THIS OK????
// IDs of the ready threads (circular queue)
int ready_list[MAX_THREADS], first_ready, last_ready, ready_siz;

// Return which thread is executing
dccthread_t* dccthread_self(void) {
	for(int i = 0; i < MAX_THREADS; i++)
		if(threads[i].executing)
			return &threads[i];
	return NULL; // no thread executing
}	

// Return name of the current thread
const char* dccthread_name(dccthread_t *tid) {
	return dccthread_self()->name;
}

// Start new thread and add to ready list
dccthread_t* dccthread_create(const char *name, void (*func)(int), int param) {
	// Finding empy place to put thread
	int i;
	for(i = 1; i < MAX_THREADS; i++)
		if(!threads[i].non_empty)
			break;
	if(i == MAX_THREADS)
		return NULL;// No more threads allowed
	dccthread_t *new_thread = &threads[i];

	// Creating thread
	new_thread->name = name;
	getcontext(&new_thread->context);
	new_thread->context.uc_link = &threads[0].context;//????????????????????????
	new_thread->context.uc_stack.ss_sp = stacks[i];
	new_thread->context.uc_stack.ss_size = sizeof stacks[i];
	new_thread->non_empty = 1;
	makecontext(&new_thread->context,(void *)func,1,param);

	// Adding to ready queue
	//if((last_ready+1)%MAX_THREADS == first_ready)
	//	return NULL; // No more threads can be ready (REDUNDANT)
	last_ready = (last_ready+1)%MAX_THREADS;
	ready_list[last_ready] = i;
	ready_siz++;

	// Return without executing the new thread
	return new_thread;
}

void dccthread_yield(void) {
	// Remove current thread from CPU and call scheduler thread to choose the next thread
	dccthread_t *curr = dccthread_self();
	curr->executing = 0;
	threads[0].executing = 1;
	swapcontext(&curr->context,&threads[0].context);
}

void scheduler() {
	
}

void dccthread_init(void (*func)(int), int param) {
	// Context to return to at the end
	ucontext_t final_context;

	// Create scheduler thread
	threads[0].name = "scheduler";
	getcontext(&threads[0].context);
	threads[0].context.uc_link = &final_context;//????????????????????????????????????????
	threads[0].context.uc_stack.ss_sp = stacks[0];
	threads[0].context.uc_stack.ss_size = sizeof stacks[0];
	threads[0].non_empty = 1;
	makecontext(&threads[0].context,(void *)scheduler,0);

	// Create main trhead that will run "func"
	dccthread_t *main_thread = dccthread_create("main",func,param);
	
	// Execute main thread
	main_thread->executing = 1;
	swapcontext(&final_context,&main_thread->context);
	exit(0);//????????????????????????????????
}
