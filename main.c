#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

// size of a memory page
#define MALLOC_BRK_SIZE 4096

// Macro used to align the allocated memory in fixed 
// memory words (usually 8 or 16 bytes)
// it is important to reduce the number of times the processor
// reads the memory, on older hardware it can cause exceptions.
//
// This macro works as a bit mask, where it rounds the number up
// and ~7 inverts the last three bits, making it multiple of 8
#define ALIGN(size) (((size) + 7) & ~7)

// This malloc stores data based on a linked list, formatted:
//
//		  _ *head (points to the start)
//		 |
//		 |           _ node data (points to the data address, calculated on real time)
//		 |          |
//		 |          |          _ node->size (size of the allocated data)
//		 |          |         |
//		 |          |         |       _ node->next (points to next node)
//		 |          |         |      |
//		 ----------------------    -----------------------
// 		|	node  |	 data   |  --  |	node  |	 data    | -- next 
//		 ----------------------	   -----------------------
//
// data address is node adress + sizeof(Node), since its stored after the node
// next node address is after the data address + data allocated size

typedef struct Node {
	size_t size;
	int status; // 0 = free, 1 = used
	struct Node *next;
} Node;

// static makes it visible only here
static Node *head;
static Node *tail;

static void* calc_data_address(Node* n);
static void merge_nodes(Node* n, Node* n2);
static void print_heap();

void *r_malloc(size_t size);
void r_free(void *pointer);

static void* calc_data_address(Node* n) {
	return (void *)((char* )n + sizeof(Node));
}

static void merge_nodes(Node *n, Node *n2) {
	n->size += n2->size + sizeof(Node);
	n->next = n2->next;
}

void *r_malloc(size_t size) {
	// on first allocation, increase program break and start 
	if (!head) {
		void *initialBrkAddress = sbrk(0);
		brk(initialBrkAddress + MALLOC_BRK_SIZE);
		head = initialBrkAddress; 

		head->size = MALLOC_BRK_SIZE - sizeof(Node); //subtract initial node
		head->status = 0;
		head->next = NULL;

		tail = head;
	}

	Node *node = head;

	size_t alignedSize = ALIGN(size);

	while (node) {
		// if the current node is used or dont have enouth space, jump to next
		if (node->status == 1 || node->size < alignedSize) {
			node = node->next;
			continue;
		}	

		size_t originalNodeSize = node->size;
		Node *originalNodeNext = node->next;

		void* dataAddress = calc_data_address(node);

		// if the original node is not the same size as the required size,
		// fragmentate the node into one with the same size and other with the remaining
		if (originalNodeSize != alignedSize) {
			// next address is saved
			// when calculation the next address by adding x bytes, it is necessary to
			// cast to char*, that is equivalent of 1 byte.
			// If sum node* + x, the compiler will add x node sizes, not x bytes
			node->size = alignedSize;
			node->next = (Node *)((char* )dataAddress + alignedSize);
			// node->prev remains the same
			
			// on the new empty node, the size of the remaining space
			// need to remove the new data and node header from the new node
			node->next->size = originalNodeSize - alignedSize - sizeof(Node); 
			node->next->status = 0;
			node->next->next = originalNodeNext;

			if (tail == node) tail = node->next;
		}

		node->status = 1;

		return dataAddress;
	}

	void *brkAddress = calc_data_address(tail) + tail->size;
	brk(brkAddress + MALLOC_BRK_SIZE);

	// In case no available nodes were found, increases the program break
	if (tail->status == 0) {
		// If the last node is free, increase its size
		tail->size += MALLOC_BRK_SIZE;
	} else {
		// If the last node is occupied, create a new one
		Node* newNode = brkAddress;
		newNode->size = MALLOC_BRK_SIZE - sizeof(Node);
		newNode->status = 0;
		newNode->next = NULL;
		tail->next = newNode;
		tail = newNode;
	}

	return r_malloc(size);
};

void r_free(void *pointer) {
	if (!head || !pointer) return;

	Node *node = head;
	Node *prevNode = NULL;

	while (node) {
		void* dataAddress = calc_data_address(node);
		if (dataAddress != pointer) {
			prevNode = node;
			node = node->next;
			continue;
		}

		node->status = 0;
		
		// Merge the current node with the previous one
		if (prevNode && prevNode->status == 0) {
			// This is for extreme cases, because the last used
			// node will hardly be the last node, there is always some
			// free space left
			if (tail == node) {
				tail = prevNode;
			}

			merge_nodes(prevNode, node);
			node = prevNode;
		}

		// Merge the current node with the next one
		if (node->next && node->next->status == 0) {
			if (tail == node->next) {
				tail = node;
			}

			merge_nodes(node, node->next);
		}

		return;
	}

	printf("Address %p not found.", pointer);
}

void print_heap() {
	printf("Head address: %p\n", head);
	printf("Tail address: %p\n", tail);

	Node *node = head;

	int i = 0;
	while (node) {
		if (node != head) {
			printf("  |\n");
		}

		char *status = node->status == 0 ? "FREE" : "USED";

		void* dataAddress = calc_data_address(node);

		printf(
			"N%d - Address: %p, Data address: %p, Size: %zu, Status: %s\n", 
			i, 
			node,
			dataAddress,
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
	*p = 1234;
	char *p2 = r_malloc(sizeof(char));
	*p2 = 's';
	print_heap();

	r_free(p2);
	r_free(p);
	print_heap();

	brk(head);
	return 0;
}
