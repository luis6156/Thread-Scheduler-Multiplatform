#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
	void *data;
	struct Node *next;
} Node;

typedef struct LinkedList {
	struct Node *head;
	struct Node *tail;
	unsigned int size;
	int (*compare_function)(void *a, void *b);
	void (*print_function)(void *a);
	void (*free_data)();
} LinkedList;

int compare_ulong(void *a, void *b);

int has_node_list(LinkedList *list, void *data);

void print_strings(void *data);

void print_ulong(void *data);

void print_list(LinkedList *list);

void add_first_node_list(LinkedList *list, void *data, size_t data_size);

void add_last_node_list(LinkedList *list, void *data, size_t data_size);

int get_size_list(LinkedList *list);

int is_empty_list(LinkedList *list);

Node *get_node_list(LinkedList *list, void *data);

void remove_node_list(LinkedList *list, void *data);

Node *pop_node_list(LinkedList *list, void *data);

void free_list(LinkedList **list);

LinkedList *initialize_list(int (*compare_function)(void *, void *),
			    void (*print_function)(void *),
			    void (*free_data)(void *));

#endif
