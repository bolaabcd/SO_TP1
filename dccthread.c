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
	char stack[MAX_BYTES_PER_STACK];
};

dccthread_t scheduler;
dccthread_t* executing;

// Circular queue for the list of ready threads. last_ready-1 is the last value. first = last if empty.
dccthread_t* ready_list[MAX_THREADS];
int first_ready, last_ready, ready_siz;

// Return which thread is executing
dccthread_t* dccthread_self(void) {
	return executing;
}	

// Return name of the current thread
const char* dccthread_name(dccthread_t *tid) {
	return dccthread_self()->name;
}

void add_ready_queue(dccthread_t* thread) {
	ready_list[last_ready] = thread;
	last_ready = (last_ready+1)%MAX_THREADS;
	if(last_ready == first_ready)
		exit(1);//??????????????????????????
	ready_siz++;
}

dccthread_t* pop_ready_queue(void) {
	if(last_ready == first_ready)
		exit(1);//???????????????????????
	dccthread_t *ans = ready_list[first_ready];
	first_ready = (first_ready+1)%MAX_THREADS;
	ready_siz--;
	return ans;
}

// Start new thread and add to ready list
dccthread_t* dccthread_create(const char *name, void (*func)(int), int param) {
	dccthread_t *new_thread = malloc(sizeof (dccthread_t));

	// Creating thread
	new_thread->name = name;
	getcontext(&new_thread->context);
	new_thread->context.uc_link = &scheduler.context;//???????????????????????
	new_thread->context.uc_stack.ss_sp = new_thread->stack;
	new_thread->context.uc_stack.ss_size = sizeof new_thread->stack;
	makecontext(&new_thread->context,(void *)func,1,param);
	
	add_ready_queue(new_thread);

	// Return without executing the new thread
	return new_thread;
}

void execute(dccthread_t *thread) {
	dccthread_t *curr = dccthread_self();
	executing = thread;
	swapcontext(&curr->context,&thread->context);
}

void dccthread_yield(void) {
	// Remove current thread from CPU and call scheduler thread to choose the next thread
	dccthread_t *curr = dccthread_self();
	
	add_ready_queue(curr);
	execute(&scheduler);
}

void schedule() {
	while(ready_siz != 0)
		execute(pop_ready_queue());
}

void dccthread_init(void (*func)(int), int param) {
	// Context to return to at the end
	dccthread_t final_thread;
	getcontext(&final_thread.context);
	final_thread.name = "dccthread_init";
	executing = &final_thread;

	// Create scheduler thread
	scheduler.name = "scheduler";
	getcontext(&scheduler.context);
	scheduler.context.uc_link = &final_thread.context;//????????????????????????????????????????
	scheduler.context.uc_stack.ss_sp = scheduler.stack;
	scheduler.context.uc_stack.ss_size = sizeof scheduler.stack;
	makecontext(&scheduler.context,schedule,0);

	// Create main thread that will run "func"
	dccthread_create("main",func,param);
	
	// Execute main thread
	execute(pop_ready_queue());
	exit(0);
}
