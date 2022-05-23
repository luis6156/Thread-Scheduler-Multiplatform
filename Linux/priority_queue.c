#include "priority_queue.h"

/**
 * @brief Compares two unsigned longs in the Priority Queue.
 *
 * @param a PQData field
 * @param b PQData field
 * @return int "0" on success
 */
int compare_ulong_pq(void *a, void *b)
{
	return !((*(unsigned long *)(((PQData *)a)->data)) ==
		 (*(unsigned long *)(((PQData *)b)->data)));
}

/**
 * @brief Frees a Node in a Priority Queue.
 *
 * @param data to be freed
 */
void free_ulong_pq(void *data)
{
	free(((PQData *)data)->data);
	free(data);
}

/**
 * @brief Prints the Priority Queue node.
 *
 * @param data node to be printed
 */
void print_ulong_pq(void *data)
{
	printf("thread: %lu p: %u, ",
	       *(unsigned long *)(((PQData *)data)->data),
	       ((PQData *)data)->priority);
}

/**
 * @brief Adds a node to the Priority Queue.
 *
 * @param pq instance of Priority Queue
 * @param data to be stored
 * @param data_size of the data
 * @param priority associated priority of the data
 */
void push_node_pq(LinkedList *pq, void *data, size_t data_size,
		  unsigned int priority)
{
	PQData pqdata;
	Node *curr, *prev, *new_node;

	if (pq == NULL || data == NULL)
		return;

	pqdata.data = malloc(data_size);
	if (!pqdata.data)
		exit(1);
	memcpy(pqdata.data, data, data_size);
	pqdata.priority = priority;

	if (is_empty_list(pq) ||
	    ((PQData *)pq->head->data)->priority < priority) {
		add_first_node_list(pq, &pqdata, sizeof(PQData));
	} else if (((PQData *)pq->tail->data)->priority >= priority) {
		add_last_node_list(pq, &pqdata, sizeof(PQData));
	} else {
		new_node = malloc(sizeof(*new_node));

		if (!new_node)
			exit(12);
		new_node->data = malloc(sizeof(PQData));

		if (!new_node->data)
			exit(12);
		memcpy(new_node->data, &pqdata, sizeof(PQData));

		prev = pq->head;
		curr = prev->next;

		while (curr != NULL) {
			if (((PQData *)prev->data)->priority == priority ||
			    ((PQData *)curr->data)->priority < priority) {
				prev->next = new_node;
				new_node->next = curr;
				break;
			}

			prev = curr;
			curr = curr->next;
		}

		pq->size++;
	}
}

/**
 * @brief Removes the top node from the Priority Queue.
 *
 * @param pq instance of Priority Queue
 */
void pop_node_pq(LinkedList *pq)
{
	Node *next, *head;

	if (pq == NULL || is_empty_list(pq))
		return;

	head = pq->head;
	next = head->next;

	pq->free_data(head->data);
	free(pq->head);
	pq->head = next;
	pq->size--;

	if (is_empty_list(pq))
		pq->tail = NULL;
}

/**
 * @brief Returns top node of the Priority Queue.
 *
 * @param pq instance of Priority Queue
 * @return PQData* node of Priority Queue
 */
PQData *peak_pq(LinkedList *pq)
{
	if (pq == NULL || is_empty_list(pq))
		return NULL;

	return (PQData *)pq->head->data;
}
