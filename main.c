#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// PERF: Now all blocks are in the linked list, not only free ones

struct block_header *free_list = NULL;
const int MIN_HEADER_SIZE = 8;
const size_t BLOCK_MAGIC = 0xDEADBEEF;

// header with metadata for the memory block
typedef struct block_header
{
	size_t size;
	bool is_free;
	struct block_header *next_block;
	size_t magic;

} block_header;

void initHeap()
{
	void *start = mmap(NULL, getpagesize(), PROT_WRITE | PROT_READ,
	                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (start == MAP_FAILED)
		perror("Map failed");

	struct block_header *header = (struct block_header *)start;

	header->next_block = NULL;
	header->is_free = true;
	header->size = getpagesize() - sizeof(struct block_header);
	header->magic = BLOCK_MAGIC;

	free_list = header;
}

void *_malloc(size_t length)
{
	if (!free_list)
		initHeap();

	if (free_list->size < length)
		printf("TODO: Create another page or smthng\n");

	struct block_header *current = free_list;

	while (current)
	{
		// skip if cant allocate in this block
		if (current->size < length || !current->is_free)
		{
			current = current->next_block;
			continue;
		}

		// allocate memory
		current->is_free = false;

		// if there is more space for the next header, place it, or dont
		size_t remaining = current->size - length;
		if (remaining >= sizeof(struct block_header) + MIN_HEADER_SIZE)
		{
			// get pointer to data area (right after the header)
			// temp + 1 skips a sizeof(block_header) as temp is a block_header*
			void *data_start = (void *)(current + 1);

			// skip length bytes and then create the new header there
			struct block_header *header =
			    (struct block_header *)((char *)data_start + length);

			// FIX: update the next_free_block pointer of temp's parent
			header->next_block = current->next_block;
			current->next_block = header;

			header->is_free = true;
			header->magic = BLOCK_MAGIC;

			header->size = remaining - sizeof(block_header);
			current->size = length;
		}

		break;
	}

	if (!current)
	{
		fprintf(stdout, "No space found");
		return NULL;
	}

	// return pointer to data section (skip the header)
	return (void *)(current + 1);
}

//
// TODO: handle cases to free pointers in the middle of the array or
// data section
// TODO: Merge with adjacent free blocks
void _free(void *data)
{
	if (!data)
		return;

	struct block_header *header = (struct block_header *)data - 1;

	if (header->magic != BLOCK_MAGIC)
	{
		fprintf(stderr, "[ERROR]: Invalid pointer or corrupted block\n");
		abort();
	}

	if (header->is_free)
	{
		fprintf(stderr, "[WARN]: Double free detected\n");
		return;
	}

	header->is_free = true;
}

int main()
{
	printf("=== Testing malloc ===\n");
	void *p1 = _malloc(64);
	void *p2 = _malloc(128);
	printf("p1: %p, p2: %p\n", p1, p2);

	printf("\n=== Testing free and reuse ===\n");
	_free(p1);
	printf("Freed p1\n");

	void *p3 = _malloc(64);
	printf("p3: %p\n", p3);
	printf("p1 == p3? %s (should be YES)\n", p1 == p3 ? "YES" : "NO");

	printf("\n=== Testing double free ===\n");
	_free(p3);
	_free(p3); // Should print warning

	printf("\n=== Testing invalid pointer ===\n");
	char *invalid = (char *)p2 + 50;
	_free(invalid); // Should abort with error
}
