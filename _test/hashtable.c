#include "hashtable.h"

#define PRIME 7
#define MAGIC_NUMBER 31

/**
 * @brief Prints the HashTable.
 *
 * @param ht hashtable to be printed
 */
void print_hashtable(HashTable *ht)
{
	unsigned int i;

	for (i = 0; i < ht->capacity; ++i)
		if (!is_empty_list(ht->lists[i]))
			print_list(ht->lists[i]);
}

/**
 * @brief Prints the key and value of a HashTable.
 *
 * @param data to be printed
 */
void print_function_entries(void *data)
{
	print_strings(((Entry *)data)->key);
	print_strings(((Entry *)data)->value);
}

/**
 * @brief Hash functions for unsigned longs.
 *
 * @param a data to be hashed
 * @return unsigned int new hash
 */
unsigned int hash_function_ulong(void *a)
{
	unsigned long x = *(unsigned long *)a;
	
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	
	return x;
}

/**
 * @brief Hash functions for strings.
 *
 * @param a data to be hashed
 * @return unsigned int new hash
 */
unsigned int hash_function_strings(void *a)
{
	unsigned int i, hash = PRIME;

	for (i = 0; i < strlen((char *)a); ++i)
		hash = hash * MAGIC_NUMBER + ((char *)a)[i];

	return hash;
}

/**
 * @brief Compares an entry key with a key.
 *
 * @param a entry key
 * @param b key
 * @return int "0" on match
 */
int compare_keys_string(void *a, void *b)
{
	return strcmp((char *)((Entry *)a)->key, (char *)b);
}

/**
 * @brief Compares two strings.
 *
 * @param a string
 * @param b string
 * @return int "0" on match
 */
int compare_strings(void *a, void *b) { return strcmp((char *)a, (char *)b); }

/**
 * @brief Free an entry in the HashTable.
 *
 * @param data entry to be freed
 */
void free_entries(void *data)
{
	Entry *entry;

	if (data == NULL)
		return;

	entry = (Entry *)data;

	free(entry->key);
	free(entry->value);
	free(entry);
}

/**
 * @brief Initializes the HashTable.
 *
 * @param capacity total capacity
 * @param hash_function used for computing indexes
 * @param compare_function used for comparing keys
 * @param print_function used for printing an entry
 * @param free_data used for freeing an entry
 * @param deep_copy_value flag to perform deep copies for the value
 * @return HashTable* new HashTable instance
 */
HashTable *initialize_hashtable(unsigned int capacity,
				unsigned int (*hash_function)(void *),
				int (*compare_function)(void *, void *),
				void (*print_function)(void *),
				void (*free_data)(void *),
				unsigned char deep_copy_value)
{
	unsigned int i;
	HashTable *ht = (HashTable *)calloc(1, sizeof(HashTable));

	if (!ht)
		exit(12);

	ht->lists = (LinkedList **)calloc(capacity, sizeof(LinkedList *));

	if (!ht->lists)
		exit(12);

	for (i = 0; i < capacity; ++i)
		ht->lists[i] = initialize_list(compare_function, print_function,
					       free_data);

	ht->capacity = capacity;
	ht->hash_function = hash_function;
	ht->compare_function = compare_function;
	ht->deep_copy_value = deep_copy_value;

	return ht;
}

/**
 * @brief Puts an entry in the HashTable.
 *
 * @param key used to compute place in HashTable
 * @param key_size of the key
 * @param value data linked to the key
 * @param value_size of the data
 * @param ht HashTable instance
 */
void put_hashtable(void *key, unsigned int key_size, void *value,
		   unsigned int value_size, HashTable *ht)
{
	Entry *tmp, entry_m;
	unsigned int index;

	if (key == NULL || value == NULL || ht == NULL)
		return;

	index = ht->hash_function(key) % ht->capacity;

	tmp = get_value_hashtable(key, ht);
	if (tmp != NULL) {
		if (ht->deep_copy_value) {
			tmp->value = realloc(tmp->value, value_size);

			if (!tmp->value)
				exit(12);
			memcpy(tmp->value, value, value_size);
		} else {
			tmp->value = value;
		}
		return;
	}

	entry_m.key = malloc(key_size);

	if (!entry_m.key)
		exit(12);
	memcpy(entry_m.key, key, key_size);

	if (ht->deep_copy_value) {
		entry_m.value = malloc(value_size);

		if (!entry_m.value)
			exit(12);

		memcpy(entry_m.value, value, value_size);
	} else {
		entry_m.value = value;
	}

	if (is_empty_list(ht->lists[index]))
		ht->size++;

	add_first_node_list(ht->lists[index], &entry_m, sizeof(entry_m));
}

/**
 * @brief Pops an entry from the HashTable.
 *
 * @param key used to find the entry
 * @param ht instance of the HashTable
 */
void remove_entry_hashtable(void *key, HashTable *ht)
{
	unsigned int index = ht->hash_function(key) % ht->capacity;

	remove_node_list(ht->lists[index], key);
}

/**
 * @brief Checks if HashTable contains an entry.
 *
 * @param key used to find the entry
 * @param ht instance of the HashTable
 * @return int "1" on success, "0" on failure, "-1" on error
 */
int has_value_hashtable(void *key, HashTable *ht)
{
	unsigned int index;
	Node *curr;

	if (key == NULL || ht == NULL)
		return -1;

	index = ht->hash_function(key) % ht->capacity;
	curr = ht->lists[index]->head;

	while (curr != NULL) {
		if (!ht->compare_function(curr->data, key))
			return 1;
		curr = curr->next;
	}

	return 0;
}

/**
 * @brief Get the value associated to a key in the HashTable.
 *
 * @param key used to find the value
 * @param ht instance of the HashTable
 * @return void* entry value
 */
void *get_value_hashtable(void *key, HashTable *ht)
{
	unsigned int index;
	Node *curr;

	if (key == NULL || ht == NULL)
		return NULL;

	index = ht->hash_function(key) % ht->capacity;
	curr = ht->lists[index]->head;

	while (curr != NULL) {
		if (!ht->compare_function(curr->data, key))
			return ((Entry *)curr->data)->value;
		curr = curr->next;
	}

	return NULL;
}

/**
 * @brief Frees the content and the HashTable itself.
 *
 * @param ht HashTable instance
 */
void free_hashtable(HashTable **ht)
{
	unsigned int i;

	if (ht == NULL || *ht == NULL)
		return;

	for (i = 0; i < (*ht)->capacity; ++i)
		free_list(&(*ht)->lists[i]);

	free((*ht)->lists);
	free(*ht);
}
