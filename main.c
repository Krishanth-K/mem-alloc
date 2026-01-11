#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

// PERF: Now all blocks are in the linked list, not only free ones

struct block_header *free_list = NULL;
const int MIN_HEADER_SIZE = 8;

// header with metadata for the memory block
typedef struct block_header
{
	size_t size;
	bool is_free;
	struct block_header *next_block;

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

int main()
{
	printf("Page size: %d\n", getpagesize());

	void *p1 = _malloc(64);
	void *p2 = _malloc(128);
	void *p3 = _malloc(256);

	printf("p1: %p\n", p1);
	printf("p2: %p\n", p2);
	printf("p3: %p\n", p3);

	printf("p2 - p1 = %ld (expect ~%zu)\n", (char *)p2 - (char *)p1,
	       64 + sizeof(block_header));
	printf("p3 - p2 = %ld (expect ~%zu)\n", (char *)p3 - (char *)p2,
	       128 + sizeof(block_header));

	// Try writing to them
	*((int *)p1) = 42;
	*((int *)p2) = 100;
	*((int *)p3) = 999;

	printf("Written values: %d, %d, %d\n", *((int *)p1), *((int *)p2),
	       *((int *)p3));
}
