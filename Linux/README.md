# Copyright (c) Micu Florian-Luis 331CA 2022 - Assignment 4

# Purpose
This assignment was made to better understand how an operating system plans its
threads using priorities and signals for io operations. Priorities must be 
maintained at all times and every single thread has a stage that it can get 
into: READY, RUNNING, WAITING, NEW, TERMINATING. After implementing this 
algorithm, I was more familiar with how threads work in Windows and Linux as
well as how to use synchronization mechanisms to only use one thread at a time.

# Implementation
## so_init
In this function, I initialize all of the data structures required to operate
my algorithm. These data structures will be detailed later. Moreover, in this
stage the priority the time quantum is set (maximum amount of time a thread
can remain on the RUNNING stage) as well as the maximum io waiting time (how
much time a thread can stay in WAITING before a signal appears).

## so_fork
Here a new thread is created by a master thread. Since the first time ever
the application is ran there will not be any threads and no priority switch
can be computed, I added a flag that will recognize this scenario and if it
is true it will simply prompt the newly created thread to run. For this
function I used a helper function so that I could better illustrate my logic:

so_fork()
    -> create thread that runs "start_thread()"
    -> decrease current thread quantum
    -> set to RUNNING the most important thread
    -> return new thread id

start_thread()
    -> wait to be prioritized
    -> run associated function
    -> set to RUNNING the most important thread since he finished

A thread switch might happen if the quantum of the RUNNING thread has
expired (since its quantum is decremented) or if a new thread with a
bigger priority has appeared.

To maintain threads in the READY state, I used a priority queue in which 
threads are pushed and popped out of. To maintain important data about
each thread I used a HashTable in which I use the thread id as key and
the attributes as values. At the end of the program each thread must be
close, therefore I used a LinkedList to maintain the ids of the threads
that have been created. In addition, the RUNNING thread is set in a struct
that holds its id and priority for future comparisons.

To signal which thread should be stuck and running, I used a semaphore
that is held in the HashTable so that any thread can get the semaphore
of another thread. The semaphore is initialized with "0" and when a thread
needs to wait, it tries to decrement its value however this will result in
a blocking manner since a semaphore cannot have a value smaller than "0".
Thus, a release must be first called (a signal must be received) to increment
the semaphore first so that it can be decremented by "wait".

## so_exec
This functions purpose is waste time of the thread, however after this
happens the program is careful to check if the thread RUNNING has not
wasted its quantum time. If this scenario is true, the thread is pushed into
the READY state and its quantum is reset. Then, the best thread is taken
and switched to the RUNNING state.

## so_wait
A signal is sent to the RUNNING thread to wait for "io" time, hence switching
the active thread with one of the READY threads. The waiting thread is stored
in a LinkedList where he will wait until its specific io signal is sent.

## so_signal
A signal is sent, therefore the program searches in the LinkedList of WAITING
threads to find all the threads that are waiting for this specific signal so
that they can be woken up. After this, they get into the READY state and the
RUNNING thread is recomputed.

## so_end
All of the launched threads are waited to be joined in this function, 
furthermore all of the memory allocated for the "so_scheduler" is freed.

### Note
1. This implementation was made to work both on Windows and Linux. The OS 
specific functions are very similar, the only difference was that closing a 
thread on Windows requires the HANDLE returned by the thread, not its id.
2. The data structures (PriorityQueue, LinkedList, HashMap) where implemented
by me from scratch and even have printing data capabilites for a more general
approach as well as for debugging if anyone uses my data structures.

# Bibliography
https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-08
https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-09
