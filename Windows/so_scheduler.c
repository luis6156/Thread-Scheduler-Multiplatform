#include "so_scheduler.h"
#include "hashtable.h"
#include "priority_queue.h"
#include <string.h>

#define HT_CAPACITY 100

typedef struct so_scheduler_t {
	HashTable *pthreads_data;	// id to pthread information
	PQData running_thread;		// current running thread
	LinkedList *ready_threads_pq;	// ready threads priority queue
	LinkedList *waiting_threads;	// waiting threads list
	LinkedList *pthreads_created;	// list of all threads created
	unsigned int time_quantum;	// time quantum for a thread
	unsigned int io;		// maximum io time
	unsigned char isAThreadRunning; // flag for first ever fork
} so_scheduler_t;

typedef struct pthread_param_t {
	HANDLE semaphore;	   // thread semaphore
	DWORD pthread_id;	   // thread id
	so_handler *func;	   // thread function
	unsigned int priority;	   // thread priority
	unsigned int time_quantum; // thread time since running
	HANDLE hThread;		   // thread handle
} pthread_param_t;

typedef struct waiting_pthread_t {
	DWORD pthread_id;      // thread id
	unsigned int priority; // thread priority
	unsigned int io;       // thread io waiting signal
} waiting_pthread_t;

so_scheduler_t so_scheduler = {0};

/**
 * @brief Used in HashTable to compare keys based on pthread.
 *
 * @param a HashTable current key represented as an "Entry" struct
 * @param b key to be searched
 * @return int "0" on success
 */
int compare_pthreads_attr(void *a, void *b)
{
	return !(*((DWORD *)((Entry *)a)->key) == *(DWORD *)b);
}

/**
 * @brief Used in LinkedList to compare node data based on thread id.
 *
 * @param a List current node represented as a "waiting_pthread_t" struct
 * @param b thread node to be searched
 * @return int "0" on success
 */
int compare_io_signal_thread(void *a, void *b)
{
	return !(((waiting_pthread_t *)a)->io == *(unsigned int *)b);
}

/**
 * @brief Used by LinkedList to prints "waiting_pthread_t" struct.
 *
 * @param data current node data
 */
void print_waiting_pthread(void *data)
{
	printf("pthread id: %lu, io: %u, priority: %u; ",
	       ((waiting_pthread_t *)data)->pthread_id,
	       ((waiting_pthread_t *)data)->io,
	       ((waiting_pthread_t *)data)->priority);
}

/**
 * @brief Used by LinkedList to prints "pthread_param_t" struct.
 *
 * @param data current node data
 */
void print_pthreads_attr(void *data)
{
	printf("pthread_id: %ld, function address: %p, "
	       "priority: %u; ",
	       ((pthread_param_t *)data)->pthread_id,
	       ((pthread_param_t *)data)->func,
	       ((pthread_param_t *)data)->priority);
}

/**
 * @brief Used by HashTable to free all entries where the key is a thread id
 * and the value a "pthread_param_t" struct.
 *
 * @param data current entry
 */
void free_entries_pthreads_attr(void *data)
{
	Entry *entry;

	if (data == NULL)
		return;

	entry = (Entry *)data;

	if (!CloseHandle(((pthread_param_t *)entry->value)->semaphore)) {
		perror("close");
		exit(1);
	};

	free(entry->key);
	free(entry->value);
	free(entry);
}

/**
 * @brief Marks the most important thread as active.
 *
 * @param max_priority_thread "PQData" structure of the most important thread
 */
void set_fastest_thread(PQData *max_priority_thread)
{
	memcpy(so_scheduler.running_thread.data, max_priority_thread->data,
	       sizeof(DWORD));
	so_scheduler.running_thread.priority = max_priority_thread->priority;
}

/**
 * @brief Set the running thread after the current thread's quantum expired.
 *
 * @param running_pthread_pararm "PQData" structure of the running thread
 */
void set_fastest_thread_after_quantum(pthread_param_t *running_pthread_pararm)
{
	PQData *max_priority_thread;
	pthread_param_t *ready_pthread_pararm;
	int ret;

	// Reset internal timer for the running thread
	running_pthread_pararm->time_quantum = so_scheduler.time_quantum;

	// Add running thread to the poll of "ready" threads
	push_node_pq(so_scheduler.ready_threads_pq,
		     &running_pthread_pararm->pthread_id, sizeof(DWORD),
		     running_pthread_pararm->priority);

	// Get max priority "ready" thread
	max_priority_thread = peak_pq(so_scheduler.ready_threads_pq);

	// Get max priority "ready" thread parameters
	ready_pthread_pararm = (pthread_param_t *)get_value_hashtable(
	    max_priority_thread->data, so_scheduler.pthreads_data);

	// Mark most important thread as running
	set_fastest_thread(max_priority_thread);

	// Remove the thread from "ready" state
	pop_node_pq(so_scheduler.ready_threads_pq);

	// Start execution for the new thread
	if (!ReleaseSemaphore(ready_pthread_pararm->semaphore, 1, NULL)) {
		perror("release");
		exit(1);
	}
	// Stop execution for the old thread
	ret = WaitForSingleObject(running_pthread_pararm->semaphore, INFINITE);
	if (ret == WAIT_FAILED) {
		perror("acquire 2");
		exit(1);
	}
}

/**
 * @brief Initializes the "so_scheduler" struct, time quantum must be > 0
 * and io at most equal to SO_MAX_NUM_EVENTS.
 *
 * @param time_quantum maximum instruction for running state
 * @param io maximum wait time for a signal
 * @return int "0" on success, "-1" on error
 */
int so_init(unsigned int time_quantum, unsigned int io)
{
	if (io > SO_MAX_NUM_EVENTS || time_quantum == 0 ||
	    so_scheduler.time_quantum != 0)
		return -1;

	// Pass internal parameters
	so_scheduler.time_quantum = time_quantum;
	so_scheduler.io = io;

	// Initialize internal data structures
	so_scheduler.pthreads_data = initialize_hashtable(
	    HT_CAPACITY, hash_function_ulong, compare_pthreads_attr,
	    print_pthreads_attr, free_entries_pthreads_attr, 0);
	so_scheduler.ready_threads_pq =
	    initialize_list(compare_ulong_pq, print_ulong_pq, free_ulong_pq);
	so_scheduler.pthreads_created =
	    initialize_list(compare_ulong, print_ulong, free);
	so_scheduler.waiting_threads = initialize_list(
	    compare_io_signal_thread, print_waiting_pthread, free);

	// Dynamic allocation for a deep copy of a thread id
	so_scheduler.running_thread.data = malloc(sizeof(DWORD));

	return 0;
}

/**
 * @brief Helper function used by a newly created thread. The thread waits to
 * be started, computes its function and lets the next thread take charge
 * after.
 *
 * @param data "pthread_param_t" structure will be passed
 * @return DWORD WINAPI NULL
 */
DWORD WINAPI start_thread(LPVOID data)
{
	int ret;
	pthread_param_t *pthread_param = (pthread_param_t *)data;

	// Waits until another thread signals that its his turn
	ret = WaitForSingleObject(pthread_param->semaphore, INFINITE);
	if (ret == WAIT_FAILED) {
		perror("acquire");
		exit(1);
	}

	// Run associated function
	pthread_param->func(pthread_param->priority);

	// Gives "running" state to next thread based on priority
	if (!is_empty_list(so_scheduler.ready_threads_pq)) {
		PQData *max_priority_thread =
		    peak_pq(so_scheduler.ready_threads_pq);
		pthread_param_t *ready_pthread_pararm =
		    (pthread_param_t *)get_value_hashtable(
			max_priority_thread->data, so_scheduler.pthreads_data);

		// Sets currently running thread
		set_fastest_thread(max_priority_thread);

		// Removes thread from "ready" state
		pop_node_pq(so_scheduler.ready_threads_pq);

		// Set running thread as active
		if (!ReleaseSemaphore(ready_pthread_pararm->semaphore, 1,
				      NULL)) {
			perror("release");
			exit(1);
		}
	}

	return 0;
}

/**
 * @brief Creates a new thread and resets the currently running thread.
 *
 * @param func function attributed to the new thread
 * @param priority of the new thread
 * @return tid_t thread id
 */
DWORD so_fork(so_handler *func, unsigned int priority)
{
	int ret;
	pthread_param_t *pthread_param;

	if (priority > SO_MAX_PRIO || func == NULL)
		return INVALID_TID;

	// Set thread parameters
	pthread_param = malloc(sizeof(pthread_param_t));
	pthread_param->func = func;
	pthread_param->priority = priority;
	pthread_param->time_quantum = so_scheduler.time_quantum;

	// Initialize thread semaphore
	pthread_param->semaphore = CreateSemaphore(NULL, 0, 1, NULL);
	if (pthread_param->semaphore == NULL) {
		perror("CreateSemaphore");
		exit(1);
	}

	// Create thread
	pthread_param->hThread =
	    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_thread,
			 pthread_param, 0, &pthread_param->pthread_id);
	if (pthread_param->hThread == NULL) {
		perror("CreateThread");
		exit(1);
	}

	// Add thread to list of all threads ever created
	add_last_node_list(so_scheduler.pthreads_created,
			   &pthread_param->hThread, sizeof(HANDLE));
	// Add thread to "ready" state priority queue
	push_node_pq(so_scheduler.ready_threads_pq, &pthread_param->pthread_id,
		     sizeof(pthread_param->pthread_id),
		     pthread_param->priority);
	// Map thread id to its properties
	put_hashtable(&pthread_param->pthread_id,
		      sizeof(pthread_param->pthread_id), pthread_param,
		      sizeof(pthread_param), so_scheduler.pthreads_data);

	// Sets to "running" the most important thread
	if (so_scheduler.isAThreadRunning) {
		// Not first ever fork -> normal procedure (get running thread
		// data)
		pthread_param_t *running_pthread_pararm =
		    (pthread_param_t *)get_value_hashtable(
			so_scheduler.running_thread.data,
			so_scheduler.pthreads_data);

		// Decrease running thread quantum
		running_pthread_pararm->time_quantum--;

		// Check if thread's quantum expired or if there are "ready"
		// threads
		if (running_pthread_pararm->time_quantum == 0) {
			set_fastest_thread_after_quantum(
			    running_pthread_pararm);
		} else if (!is_empty_list(so_scheduler.ready_threads_pq)) {
			PQData *max_priority_thread =
			    peak_pq(so_scheduler.ready_threads_pq);

			// Check if the current thread does not have the
			// biggest priority
			if (max_priority_thread->priority >
			    so_scheduler.running_thread.priority) {
				// Get replacer's data
				pthread_param_t *ready_pthread_pararm =
				    (pthread_param_t *)get_value_hashtable(
					max_priority_thread->data,
					so_scheduler.pthreads_data);

				// Set new thread to "running" state
				set_fastest_thread(max_priority_thread);

				// Remove new thread from "ready" state
				pop_node_pq(so_scheduler.ready_threads_pq);

				// Set the previous thread to "ready" state
				push_node_pq(
				    so_scheduler.ready_threads_pq,
				    &running_pthread_pararm->pthread_id,
				    sizeof(DWORD),
				    running_pthread_pararm->priority);

				// Start execution for the new thread
				if (!ReleaseSemaphore(
					ready_pthread_pararm->semaphore, 1,
					NULL)) {
					perror("release");
					exit(1);
				}
				// Stop execution for the old thread
				ret = WaitForSingleObject(
				    running_pthread_pararm->semaphore,
				    INFINITE);
				if (ret == WAIT_FAILED) {
					perror("acquire 4");
					exit(1);
				}
			}
		}
	} else {
		PQData *max_priority_thread;

		// Mark the first ever fork as true
		so_scheduler.isAThreadRunning = 1;

		// Set the new thread directly to "running" state
		max_priority_thread = peak_pq(so_scheduler.ready_threads_pq);
		set_fastest_thread(max_priority_thread);

		// Remove it from "ready" state
		pop_node_pq(so_scheduler.ready_threads_pq);
		// Start execution for the new thread
		if (!ReleaseSemaphore(pthread_param->semaphore, 1, NULL)) {
			perror("release");
			exit(1);
		}
	}

	return pthread_param->pthread_id;
}

/**
 * @brief Wastes thread quantum time and resets the "running" thread if
 * necessary.
 *
 */
void so_exec(void)
{
	pthread_param_t *running_pthread_pararm;

	if (!so_scheduler.isAThreadRunning)
		return;

	// Decrease current thread's quantum
	running_pthread_pararm = (pthread_param_t *)get_value_hashtable(
	    so_scheduler.running_thread.data, so_scheduler.pthreads_data);
	running_pthread_pararm->time_quantum--;

	// Check if thread's quantum expired
	if (running_pthread_pararm->time_quantum == 0)
		set_fastest_thread_after_quantum(running_pthread_pararm);
}

/**
 * @brief Marks the "running" thread to "waiting" state and sets the next
 * thread from the "ready" priority queue to run.
 *
 * @param io signal to be waiting for when "so_signal" hits
 * @return int "0" on success, "-1" on error
 */
int so_wait(unsigned int io)
{
	int ret;
	pthread_param_t *running_pthread_pararm;
	waiting_pthread_t waiting_pthread;

	if (!so_scheduler.isAThreadRunning)
		return 0;

	if (io >= so_scheduler.io)
		return -1;

	// Get running thread's data
	running_pthread_pararm = (pthread_param_t *)get_value_hashtable(
	    so_scheduler.running_thread.data, so_scheduler.pthreads_data);

	// Set "waiting_pthread_t" struct attributes
	waiting_pthread.pthread_id = running_pthread_pararm->pthread_id;
	waiting_pthread.priority = running_pthread_pararm->priority;
	waiting_pthread.io = io;

	// Set thread state to "waiting"
	add_last_node_list(so_scheduler.waiting_threads, &waiting_pthread,
			   sizeof(waiting_pthread_t));

	// Check if there are "ready" threads
	if (!is_empty_list(so_scheduler.ready_threads_pq)) {
		// Get best thread available
		PQData *max_priority_thread =
		    peak_pq(so_scheduler.ready_threads_pq);
		pthread_param_t *ready_pthread_pararm =
		    (pthread_param_t *)get_value_hashtable(
			max_priority_thread->data, so_scheduler.pthreads_data);

		// Mark new thread as "running"
		set_fastest_thread(max_priority_thread);

		// Remove new thread from "ready" state
		pop_node_pq(so_scheduler.ready_threads_pq);

		// Start execution for the new thread
		if (!ReleaseSemaphore(ready_pthread_pararm->semaphore, 1,
				      NULL)) {
			perror("release");
			exit(1);
		}
	}

	// Stop execution for the old thread
	ret = WaitForSingleObject(running_pthread_pararm->semaphore, INFINITE);
	if (ret == WAIT_FAILED) {
		perror("acquire");
		exit(1);
	}

	return 0;
}

/**
 * @brief Marks the waiting threads waiting for the io signal as "ready" from
 * "waiting", also resets the "running" thread.
 *
 * @param io signal to be used to unlock threads
 * @return int number of threads woken or "-1" on error
 */
int so_signal(unsigned int io)
{
	int ret;
	pthread_param_t *running_pthread_pararm;
	PQData *max_priority_thread;
	Node *thread_data;
	pthread_param_t *ready_pthread_pararm;
	int num_threads = 0;

	if (io >= so_scheduler.io)
		return -1;

	if (!so_scheduler.isAThreadRunning ||
	    is_empty_list(so_scheduler.waiting_threads))
		return 0;

	// Get "running" thread data
	running_pthread_pararm = (pthread_param_t *)get_value_hashtable(
	    so_scheduler.running_thread.data, so_scheduler.pthreads_data);

	// Mark "running" thread as "ready"
	push_node_pq(so_scheduler.ready_threads_pq,
		     &running_pthread_pararm->pthread_id, sizeof(DWORD),
		     running_pthread_pararm->priority);

	// Signal all threads that have the "io" signal to be set to "ready"
	thread_data = pop_node_list(so_scheduler.waiting_threads, &io);

	while (thread_data != NULL) {
		push_node_pq(
		    so_scheduler.ready_threads_pq,
		    &((waiting_pthread_t *)thread_data->data)->pthread_id,
		    sizeof(DWORD),
		    ((waiting_pthread_t *)thread_data->data)->priority);

		free(thread_data->data);
		free(thread_data);

		thread_data = pop_node_list(so_scheduler.waiting_threads, &io);
		num_threads++;
	}

	// Get most important thread data
	max_priority_thread = peak_pq(so_scheduler.ready_threads_pq);
	ready_pthread_pararm = (pthread_param_t *)get_value_hashtable(
	    max_priority_thread->data, so_scheduler.pthreads_data);

	// Mark new thread as "running"
	set_fastest_thread(max_priority_thread);

	// Remove the new thread from "ready" state
	pop_node_pq(so_scheduler.ready_threads_pq);

	// Start execution for the new thread
	if (!ReleaseSemaphore(ready_pthread_pararm->semaphore, 1, NULL)) {
		perror("release");
		exit(1);
	}
	// Stop execution for the old thread
	ret = WaitForSingleObject(running_pthread_pararm->semaphore, INFINITE);
	if (ret == WAIT_FAILED) {
		perror("acquire");
		exit(1);
	}

	return num_threads;
}

/**
 * @brief Waits for all threads to wait and frees "so_scheduler" struct.
 *
 */
void so_end(void)
{
	HANDLE hThread;
	int ret;
	Node *curr;
	// Waits for all ever created threads to finish
	if (so_scheduler.pthreads_created != NULL) {
		curr = so_scheduler.pthreads_created->head;
		while (curr != NULL) {
			hThread = *(HANDLE *)(curr->data);
			ret = WaitForSingleObject(hThread, INFINITE);
			if (ret == WAIT_FAILED) {
				perror("pthread_join");
				exit(1);
			}
			curr = curr->next;
		}
	}

	// Free all internal structures
	free_list(&so_scheduler.pthreads_created);
	free_list(&so_scheduler.ready_threads_pq);
	free_list(&so_scheduler.waiting_threads);
	free_hashtable(&so_scheduler.pthreads_data);
	free(so_scheduler.running_thread.data);

	// Sets all the struct's field to "0" for safety
	memset(&so_scheduler, 0, sizeof(so_scheduler_t));
}
