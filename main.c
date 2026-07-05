#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#define MALLOC_BRK_SIZE 1000

// This malloc stores data based on a linked list, formatted:
//
//  _ *head (points to the start)
// |
// |           _ node->data (points to the data address)
// |          |
// |          |          _ node->size (size of the allocated data)
// |          |         |
// |          |         |       _ node->next (points to next node)
// |          |         |      |
// ----------------------      -----------------------
// |	node  |	 data   |  --  |	node  |	 data    | -- NULL
// ----------------------	   -----------------------
//
// data address is node adress + sizeof(Node), since its stored after the node
// next node address is after the data address + data allocated size

typedef struct Node {
	size_t size;
	int status; // 0 = free, 1 = used
	void *data;
	struct Node *next;
	struct Node *prev;
} Node;

Node *head;

void *r_malloc(size_t size);
void print_heap();
void r_free(void *pointer);

void *r_malloc(size_t size) {
	// on first allocation, increase program break and start 
	if (!head) {
		void *initialBrkAddress = sbrk(0);
		brk(initialBrkAddress + MALLOC_BRK_SIZE);
		head = initialBrkAddress; 

		head->size = MALLOC_BRK_SIZE - sizeof(Node); //subtract initial node
		head->status = 0;
		head->next = NULL;
		head->prev = NULL;
		head->data = (void *) ((char *)head + sizeof(Node));
	}

	Node *node = head;

	while (node) {
		// if the current node is used or dont have enouth space, jump to next
		if (node->status == 1 || node->size < size) {
			node = node->next;
			continue;
		}	

		size_t originalNodeSize = node->size;
		Node *originalNodeNext = node->next;

		// if the original node is not the same size as the required size,
		// fragmentate the node into one with the same size and other with the remaining
		if (originalNodeSize != size) {
			// next address is saved
			// when calculation the next address by adding x bytes, it is necessary to
			// cast to char*, that is equivalent of 1 byte.
			// If sum node* + x, the compiler will add x node sizes, not x bytes
			node->size = size;
			node->data = (void *)((char* )node + sizeof(Node));
			node->next = (Node *)((char* )node->data + size);
			// node->prev remains the same
			
			// on the new empty node, the size of the remaining space
			// need to remove the new data and node header from the new node
			node->next->size = originalNodeSize - size - sizeof(Node); 
			node->next->data = (void *)((char* )node->next + sizeof(Node));
			node->next->status = 0;
			node->next->next = originalNodeNext;
			node->next->prev = node;
		}

		node->status = 1;

		return node->data;
	}

	return NULL;
};

void r_free(void *pointer) {
	if (!head || !pointer) return;

	Node *node = head;

	while (node) {
		if (node->data != pointer) {
			node = node->next;
			continue;
		}

		node->status = 0;
		
		if (node->prev && node->prev->status == 0) {
			node->prev->size += node->size + sizeof(Node);
			node->prev->next = node->next;
			node = node->prev;
		}

		if (node->next && node->next->status == 0) {
			node->size += node->next->size + sizeof(Node);
			node->next = node->next->next;
		}

		return;
	}

	printf("Address %p not found.", pointer);
}

void print_heap() {
	printf("Head address: %p\n", head);

	Node *node = head;

	int i = 0;
	while (node) {
		if (node != head) {
			printf("  |\n");
		}

		char *status = node->status == 0 ? "FREE" : "USED";

		printf(
			"N%d - Address: %p, Data address: %p, Size: %zu, Status: %s\n", 
			i, 
			node,
			node->data,
			node->size,
			status
		);

		node = node->next;
		i++;
	}
	printf("\n");
}

int main() {
	int *p = r_malloc(sizeof(int));
	*p = 100;

	print_heap();

	char *p2 = r_malloc(sizeof(char));
	*p2 = 'a';

	print_heap();

	r_free(p2);
	print_heap();


	r_free(p);
	print_heap();


	brk(head);
	return 0;
}
