#include "mymalloc.h"

typedef struct Flist {
	size_t size;
	struct Flist *next;
} flist;

flist *head = NULL;

void *my_malloc(size_t size) {
	/* Give the user a pointer to an allocated chunk of memory whose size 
	 * is at least that of the given size plus 8 bytes for bookkeeping. */

	if (size <= 0) return NULL;

	size_t chunk_size = (size + 15) & -8; // Align size + bookkeeping to 8 bytes.
	size_t sbrk_min = 8192, sbrk_size = sbrk_min;

	// This line is potentially problematic. Consider removing + sizeof(flist) if things go wrong.
	if (chunk_size > sbrk_min) sbrk_size = chunk_size; 

	// Case of first malloc in program.
	if (head == NULL) {
		head = (flist *) sbrk(sbrk_size);
		head->size = sbrk_size;
		head->next = NULL;
	}

	/* Search for free list node with enough available space. If none found,
	 * create a new node with sbrk(). */
	flist *curr_ptr = head;

	while (curr_ptr->size < chunk_size) {
		
		if (curr_ptr->next != NULL) {
			curr_ptr = curr_ptr->next;

		} else {
			// Create and link newly made pointer.
			curr_ptr->next = (flist *) sbrk(sbrk_size);
			curr_ptr = curr_ptr->next;
			curr_ptr->next = NULL;
			curr_ptr->size = sbrk_size;
		}
	}

	if (curr_ptr->size >= chunk_size + 8) {
		// Sufficient memory remains, slice from end of node.

		// Determine address of starting point of data.
		char *data_loc = (char *) curr_ptr;
		data_loc += curr_ptr->size;

		data_loc -= chunk_size;
		*((int *) data_loc) = chunk_size;
		data_loc += 8;
		curr_ptr->size -= chunk_size;

		return (void *) data_loc;
	
	} else {
		// Request will consume entire chunk. Give whole node and remove from list.
		*((int *) curr_ptr) = chunk_size;

		if (curr_ptr == head) { 
			head = head->next;

		} else {
			// Iterate through free list and disconnect full node.
			flist *temp = head;

			while (temp->next != curr_ptr) {
				temp = temp->next;
			}

			temp->next = temp->next->next;
		}

		return ((void *) curr_ptr) + 8;
	}
}

void my_free(void *ptr) {
	// Add chunk at ptr to the free list.

	if (ptr == NULL) return;

	flist *curr_node = free_list_begin();

	flist *temp = head;

	head = ptr - 8;
	head->next = temp;
}

void *free_list_begin() {
	// Return the start of the free list.
	return head;
}

void *free_list_next(void *node) {
	// Return the free list node right after given node.

	if (node == NULL) return NULL;

	return ((flist *) node)->next; 
}

void coalesce_free_list() {
	// Combine contiguous nodes in the free list to reduce number 
	// of nodes and increase memory available to the user.
	
	// Count nodes in free list.
	flist *curr_node = free_list_begin();
	int node_count = 0;

	while (curr_node != NULL) {
		node_count++;
		curr_node = curr_node->next;
	}

	// Store node addresses in array.
	unsigned int nodes[node_count], nodes_sorted[node_count], i, j;
	curr_node = free_list_begin();

	for (i = 0; i < node_count; i++) {
		nodes[i] = (unsigned int) curr_node;
		curr_node = curr_node->next;
	}

	// Sort node list in ascending order.
	unsigned int min = nodes[0], min_index = 0;
	
	for (i = 0; i < node_count; i++) {

		for (j = 0; j < node_count; j++) {
			
			if (nodes[j] <= min) {
				min = nodes[j];
				min_index = j;
			}
		}

		nodes[min_index] = -1;
		nodes_sorted[i] = min;
		min = -1;

		// Relink nodes to make coallescing easier.
		if (i != 0) ((flist *) nodes_sorted[i - 1])->next = (flist *) nodes_sorted[i];
	}

	head = (flist *) nodes_sorted[0];
	((flist *) nodes_sorted[node_count - 1])->next = NULL;

	curr_node = free_list_begin();
	flist *tested_node;

	// Traverse free list, checking following nodes.
	while (curr_node != NULL) {

		if (curr_node->size == 0 || curr_node->next == NULL) {
			curr_node = curr_node->next;
			continue;
		}

		tested_node = curr_node->next;

		// Merge if chunks are adjacent.
		while ( ((void *) curr_node) + curr_node->size == tested_node) {
			curr_node->next = tested_node->next;
			curr_node->size += tested_node->size;
			tested_node->size = 0; // Flag to avoid checking node in future iterations.
			tested_node = tested_node->next;
		}

		curr_node = curr_node->next;
	}

}
