#include "linkedlist.h"

/**
 * @brief Compares longs.
 *
 * @param a long
 * @param b long
 * @return int "0" on match
 */
int compare_ulong(void *a, void *b)
{
	return *(unsigned long *)a > *(unsigned long *)b;
}

/**
 * @brief Adds a node to the start of the list.
 *
 * @param list source to be added
 * @param data to be added
 * @param data_size of the data
 */
void add_first_node_list(LinkedList *list, void *data, size_t data_size)
{
	Node *new_node;

	if (data == NULL || list == NULL)
		return;

	new_node = malloc(sizeof(*new_node));

	if (!new_node)
		exit(12);
	new_node->data = malloc(data_size);

	if (!new_node->data)
		exit(12);
	memcpy(new_node->data, data, data_size);

	if (is_empty_list(list)) {
		new_node->next = NULL;
		list->head = new_node;
		list->tail = new_node;
	} else {
		new_node->next = list->head;
		list->head = new_node;
	}

	list->size++;
}

/**
 * @brief Adds a node to the end of the list.
 *
 * @param list source to be added
 * @param data to be added
 * @param data_size of the data
 */
void add_last_node_list(LinkedList *list, void *data, size_t data_size)
{
	Node *new_node;

	if (data == NULL || list == NULL)
		return;

	new_node = malloc(sizeof(*new_node));

	if (!new_node)
		exit(12);
	new_node->data = malloc(data_size);

	if (!new_node->data)
		exit(12);
	memcpy(new_node->data, data, data_size);
	new_node->next = NULL;

	if (is_empty_list(list)) {
		list->head = new_node;
		list->tail = new_node;
	} else {
		list->tail->next = new_node;
		list->tail = new_node;
	}

	list->size++;
}

/**
 * @brief Get the size of the list.
 *
 * @param list source
 * @return int size or "-1" on error
 */
int get_size_list(LinkedList *list)
{
	if (list == NULL)
		return -1;

	return list->size;
}

/**
 * @brief Checks if list is empty.
 *
 * @param list source
 * @return int "1" for true, "0" for false, "-1" on error
 */
int is_empty_list(LinkedList *list)
{
	if (list == NULL)
		return -1;

	return get_size_list(list) == 0;
}

/**
 * @brief Checks if list has node.
 *
 * @param list source
 * @param data to be checked in list
 * @return int "1" for true, "0" for false, "-1" on error
 */
int has_node_list(LinkedList *list, void *data)
{
	Node *curr;

	if (data == NULL || list == NULL)
		return -1;

	curr = list->head;

	while (curr != NULL) {
		if (!list->compare_function(curr->data, data))
			return 1;
		curr = curr->next;
	}

	return 0;
}

/**
 * @brief Get a node in a list.
 *
 * @param list source
 * @param data to be searched for
 * @return Node* that contains the data
 */
Node *get_node_list(LinkedList *list, void *data)
{
	Node *curr;

	if (data == NULL || list == NULL)
		return NULL;

	curr = list->head;

	while (curr != NULL) {
		if (!list->compare_function(curr->data, data))
			return curr;

		curr = curr->next;
	}

	return NULL;
}

/**
 * @brief Get a node in a list and pops it from the list.
 *
 * @param list source
 * @param data to be searched for
 * @return Node* that contains the data
 */
Node *pop_node_list(LinkedList *list, void *data)
{
	Node *prev, *curr;

	if (data == NULL || list == NULL || is_empty_list(list))
		return NULL;

	if (get_size_list(list) == 1) {
		if (!list->compare_function(list->head->data, data)) {
			curr = list->head;
			list->head = NULL;
			list->tail = NULL;
			list->size--;
			return curr;
		} else {
			return NULL;
		}
	}

	if (!list->compare_function(list->head->data, data)) {
		curr = list->head;
		list->head = list->head->next;
		list->size--;
		return curr;
	}

	prev = list->head;
	curr = list->head->next;
	while (curr != list->tail) {
		if (!list->compare_function(curr->data, data)) {
			prev->next = curr->next;
			list->size--;
			return curr;
		}
		prev = curr;
		curr = curr->next;
	}

	if (!list->compare_function(list->tail->data, data)) {
		curr = list->tail;

		prev->next = NULL;
		list->tail = prev;
		list->size--;
		return curr;
	}

	return NULL;
}

/**
 * @brief Removes a node from the list.
 *
 * @param list
 * @param data
 */
void remove_node_list(LinkedList *list, void *data)
{
	Node *prev, *curr;

	if (data == NULL || list == NULL || is_empty_list(list))
		return;

	if (get_size_list(list) == 1 &&
	    !list->compare_function(list->head->data, data)) {
		list->free_data(list->head->data);
		free(list->head);
		list->head = NULL;
		list->tail = NULL;
		list->size--;
		return;
	}

	if (!list->compare_function(list->head->data, data)) {
		Node *tmp = list->head;

		list->head = list->head->next;
		list->size--;
		list->free_data(tmp->data);
		free(tmp);
		return;
	}

	prev = list->head;
	curr = list->head->next;
	while (curr != list->tail) {
		if (!list->compare_function(curr->data, data)) {
			prev->next = curr->next;
			list->size--;
			list->free_data(curr->data);
			free(curr);
			return;
		}
		prev = curr;
		curr = curr->next;
	}

	if (!list->compare_function(list->tail->data, data)) {
		Node *tmp = list->tail;

		prev->next = NULL;
		list->tail = prev;
		list->size--;
		list->free_data(tmp->data);
		free(tmp);
	}
}

/**
 * @brief Prints the list
 *
 * @param list to be printed
 */
void print_list(LinkedList *list)
{
	Node *curr = list->head;

	while (curr != NULL) {
		list->print_function(curr->data);
		curr = curr->next;
	}

	printf("\n");
}

/**
 * @brief Prints unsigned longs.
 *
 * @param data to be printed
 */
void print_ulong(void *data) { printf("%lu ", *(unsigned long *)data); }

/**
 * @brief Prints strings.
 *
 * @param data to be printed
 */
void print_strings(void *data) { printf("%s ", (char *)data); }

/**
 * @brief Frees a list.
 *
 * @param list to be freed
 */
void free_list(LinkedList **list)
{
	Node *curr;

	if (*list == NULL)
		return;

	curr = (*list)->head;

	while (curr != NULL) {
		Node *tmp = curr;

		curr = curr->next;
		(*list)->free_data(tmp->data);
		free(tmp);
	}

	free(*list);
}

/**
 * @brief Initializes a list.
 *
 * @param compare_function to be used for searching a node
 * @param print_function to be used for printing a node
 * @param free_data to be used for freeing a node
 * @return LinkedList* new list instance
 */
LinkedList *initialize_list(int (*compare_function)(void *, void *),
			    void (*print_function)(void *),
			    void (*free_data)(void *))
{
	LinkedList *list = calloc(1, sizeof(*list));

	if (!list)
		exit(12);

	list->compare_function = compare_function;
	list->print_function = print_function;
	list->free_data = free_data;

	return list;
}
