#include "dccthread.h"
#include "dlist.h"
#include <ucontext.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <signal.h>
#include <stdio.h>

// Maximum stack size of each thread.
#define MAX_BYTES_PER_STACK 8192

// Structure with data for the threads
struct dccthread {
	const char* name;
	ucontext_t context;
	char stack[MAX_BYTES_PER_STACK];
	dccthread_t* waiting_thread;
	int completed;
};

dccthread_t scheduler;
dccthread_t* executing;
sigset_t sigrt_both;

int sleeping;

// Circular queue for the list of ready threads
struct dlist* ready_queue;

// Returns which thread is executing
dccthread_t* dccthread_self(void) {
	return executing;
}	

// Returns the name of the current thread
const char* dccthread_name(dccthread_t *tid) {
	return tid->name;
}

void add_ready_queue(dccthread_t* thread) {
	dlist_push_right(ready_queue, (void*) thread);
}

dccthread_t* pop_ready_queue(void) {
	assert(!dlist_empty(ready_queue));
	dccthread_t* thread = (dccthread_t*) dlist_get_index(ready_queue, 0);
	dlist_pop_left(ready_queue);

	return thread;
}

// Starts new thread and adds it to the ready list
dccthread_t* dccthread_create(const char *name, void (*func)(int), int param) {
	dccthread_t *new_thread = malloc(sizeof (dccthread_t));

	// Creating thread
	char* buffer = malloc((strlen(name) + 1) * sizeof(char));
	strcpy(buffer, name);
	new_thread->name = buffer; // this copy is necessary (test11.sh changes the contents of "name")

	new_thread->completed = 0;
	new_thread->waiting_thread = NULL;
	getcontext(&new_thread->context);
	new_thread->context.uc_link = &scheduler.context;
	new_thread->context.uc_stack.ss_sp = new_thread->stack;
	new_thread->context.uc_stack.ss_size = sizeof new_thread->stack;
	makecontext(&new_thread->context, (void*) func, 1, param);
	
	// We must block after getcontext!!!
	sigprocmask(SIG_BLOCK, &sigrt_both, NULL);
	add_ready_queue(new_thread);

	// Returning without executing the new thread
	sigprocmask(SIG_UNBLOCK, &sigrt_both, NULL);
	return new_thread;
}

void execute(dccthread_t *thread) {
	dccthread_t *curr = dccthread_self();
	executing = thread;
	swapcontext(&curr->context, &thread->context);
}

void dccthread_yield(void) {
	sigprocmask(SIG_BLOCK, &sigrt_both, NULL);
	// Removing current thread from CPU and call scheduler thread to choose the next thread
	dccthread_t *curr = dccthread_self();
	
	add_ready_queue(curr);

	execute(&scheduler);
	sigprocmask(SIG_UNBLOCK, &sigrt_both, NULL);
}

void schedule() {
	sigprocmask(SIG_BLOCK, &sigrt_both, NULL);
	while (!dlist_empty(ready_queue) || sleeping != 0) {
		if (!dlist_empty(ready_queue))
			execute(pop_ready_queue());
	}
}

void dccthread_sighandler(int signum) {
	assert(signum == SIGRTMIN);
	dccthread_yield();
}

// Adds the thread that just woke up to the ready queue
void dccthread_sighandler_sleep(int signum, siginfo_t *info, void* context) {
	assert(signum == SIGRTMAX);
	sigprocmask(SIG_BLOCK, &sigrt_both, NULL);
	add_ready_queue(info->si_value.sival_ptr);
	sigprocmask(SIG_UNBLOCK, &sigrt_both, NULL);
}

void dccthread_init(void (*func)(int), int param) {
	// Creating ready queue
	ready_queue = dlist_create();

	// Create scheduler thread
	scheduler.name = "scheduler";
	scheduler.completed = 0;
	scheduler.waiting_thread = NULL;
	getcontext(&scheduler.context);
	scheduler.context.uc_link = NULL;
	scheduler.context.uc_stack.ss_sp = scheduler.stack;
	scheduler.context.uc_stack.ss_size = sizeof scheduler.stack;
	makecontext(&scheduler.context,schedule,0);

	// Create main thread that will run "func"
	dccthread_create("main", func, param);

	// Creating timer for timed preemption
	struct sigevent sev;
	timer_t timerid;
	struct itimerspec spec;

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGRTMIN;
	sev.sigev_value.sival_ptr = NULL;
	assert(timer_create(CLOCK_PROCESS_CPUTIME_ID, &sev, &timerid) != -1);
	
	spec.it_value.tv_sec = 0;
	spec.it_value.tv_nsec = 10000000;
	spec.it_interval.tv_sec = 0;
	spec.it_interval.tv_nsec = 10000000;

	// Specifying how to handle timed preemption signal (we'll use SIGRTMIN)
	struct sigaction action, action_sleep;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	action.sa_handler = dccthread_sighandler;

	// Setting SIGRTMIN signal action
	assert(sigaction(SIGRTMIN, &action, NULL) != -1);

	// Specifying sigset with both SIGRTMIN and SIGRTMAX 
	sigemptyset(&sigrt_both);
	assert(sigaddset(&sigrt_both, SIGRTMIN) != -1);
	assert(sigaddset(&sigrt_both, SIGRTMAX) != -1);

	// Specifying how to handle dccthread_wait signal (we'll use SIGRTMAX)
	action_sleep.sa_flags = SA_SIGINFO;
	action_sleep.sa_sigaction = dccthread_sighandler_sleep;
	sigemptyset(&action_sleep.sa_mask);

	
	// Setting SIGRTMAX signal action
	assert(sigaction(SIGRTMAX, &action_sleep, NULL) != -1);
	
	// Starting timed preemption timer
	assert(timer_settime(timerid, 0, &spec, NULL) != -1);

	// Executing main thread
	while(1) {
		executing = &scheduler;
		setcontext(&scheduler.context);
	}
}


void dccthread_exit(void) {
	sigprocmask(SIG_BLOCK, &sigrt_both, NULL);
	dccthread_t* curr = dccthread_self();
	curr->completed = 1;
	if (curr->waiting_thread != NULL) {
		execute(curr->waiting_thread);
	}
	sigprocmask(SIG_UNBLOCK, &sigrt_both, NULL);
	free(dccthread_self());
}

void dccthread_wait(dccthread_t *tid) {
	while (!tid->completed) {
		dccthread_yield();
	}
}

void dccthread_sleep(struct timespec ts) {
	sigprocmask(SIG_BLOCK, &sigrt_both, NULL);
	// Creating timer for dccthread_sleep
	timer_t timer_sleep;
	struct sigevent sev_sleep;
	struct itimerspec spec_sleep;
	sev_sleep.sigev_notify = SIGEV_SIGNAL;
	sev_sleep.sigev_signo = SIGRTMAX;
	sev_sleep.sigev_value.sival_ptr = dccthread_self();
	assert(timer_create(CLOCK_REALTIME, &sev_sleep, &timer_sleep) != -1);

	// Creating specification for dccthread_sleep timer
	spec_sleep.it_value = ts;
	spec_sleep.it_interval.tv_sec = 0;
	spec_sleep.it_interval.tv_nsec = 0;

	// Setting timer
	assert(timer_settime(timer_sleep, 0, &spec_sleep, NULL) != -1);

	sleeping++;
	execute(&scheduler);
	sleeping--;
	sigprocmask(SIG_UNBLOCK, &sigrt_both, NULL);
}
