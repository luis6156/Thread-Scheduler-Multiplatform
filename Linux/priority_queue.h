#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "linkedlist.h"

typedef struct PQData {
	void *data;
	unsigned int priority;
} PQData;

int compare_ulong_pq(void *a, void *b);

void free_ulong_pq(void *data);

void print_ulong_pq(void *data);

void push_node_pq(LinkedList *pq, void *data, size_t data_size,
		  unsigned int priority);

void pop_node_pq(LinkedList *pq);

PQData *peak_pq(LinkedList *pq);

#endif
