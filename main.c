#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// PERF: Now all blocks are in the linked list, not only free ones

struct block_header *free_list = NULL;
const int MIN_HEADER_SIZE = 8;
const size_t BLOCK_MAGIC = 0xDEADBEEF;

// header with metadata for the memory block
typedef struct block_header
{
	size_t size;
	bool is_free;
	size_t magic;

	struct block_header *prev;
	struct block_header *next;

} block_header;

// create a new page and initialize a header and return it
struct block_header *getHeap()
{
	void *start = mmap(NULL, getpagesize(), PROT_WRITE | PROT_READ,
	                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (start == MAP_FAILED)
	{
		perror("Map failed");
		exit(1);
	}

	struct block_header *header = (struct block_header *)start;

	header->size = getpagesize() - sizeof(struct block_header);
	header->is_free = true;
	header->magic = BLOCK_MAGIC;

	header->prev = NULL;
	header->next = NULL;

	return header;
}

void initHeap()
{
	struct block_header *header = getHeap();
	free_list = header;
}

void coalesce(struct block_header *current)
{

	// merge with next header
	if (current->next && current->next->is_free)
	{
		struct block_header *next = current->next;

		current->size += next->size + sizeof(struct block_header);
		current->next = next->next;

		if (next->next)
			next->next->prev = current;
	}

	// merge with prev header
	if (current->prev && current->prev->is_free)
	{
		struct block_header *prev = current->prev;

		prev->size += current->size + sizeof(struct block_header);
		prev->next = current->next;

		if (current->next)
			current->next->prev = prev;

		// now current points to garbage inside the combined data section
	}
}

void expandHeap()
{
	struct block_header *new_page_block = getHeap();

	// find the last block in the free list
	struct block_header *current = free_list;

	if (!free_list)
	{
		free_list = new_page_block;
		return;
	}

	while (current->next)
		current = current->next;

	// attach the last block with the new page block
	current->next = new_page_block;
	new_page_block->prev = current;

	// coalesce them if possible
	if (current->is_free)
		coalesce(current);
}

void *_malloc(size_t length)
{
	if (!free_list)
		initHeap();

	// align the length
	length = ALIGN(length);

	struct block_header *current = free_list;

	while (current)
	{
		// skip if cant allocate in this block
		if (current->size < length || !current->is_free)
		{
			current = current->next;
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
			struct block_header *new_block =
			    (struct block_header *)((char *)data_start + length);

			// FIX: update the next_free_block pointer of temp's parent
			new_block->next = current->next;
			current->next = new_block;

			// update the prev pointers
			new_block->prev = current;
			if (new_block->next)
				new_block->next->prev = new_block;

			new_block->is_free = true;
			new_block->magic = BLOCK_MAGIC;

			new_block->size = remaining - sizeof(block_header);
			current->size = length;
		}

		break;
	}

	// if no space in current page, create a new page and then allocate
	if (!current)
	{
		expandHeap();
		return _malloc(length);
	}

	// return pointer to data section (skip the header)
	return (void *)(current + 1);
}

//
// TODO: handle cases to free pointers in the middle of the array or
// data section
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

	coalesce(header);
}

void *_calloc(size_t num, size_t size)
{
	size_t total = num * size;

	// check for overflow
	if (num != 0 && total / num != size)
	{
		fprintf(stderr, "[ERROR]: Integer overflow during calloc\n");
		return NULL;
	}

	// initialize the memory
	void *data = _malloc(total);

	if (!data)
	{
		fprintf(stderr, "[ERROR]: _malloc failed!\n");
		return NULL;
	}

	// set initial values to 0
	memset(data, 0, total);

	return data;
}

// create a new space in memory for "size" bytes and then copy the content
// over from "ptr"
void *_realloc(void *ptr, size_t size)
{
	size = ALIGN(size);

	// explicitly allowed
	if (!ptr)
		return _malloc(size);

	// a valid pointer and a size == 0 is equivalent to free(ptr)
	if (size == 0)
	{
		_free(ptr);
		return NULL;
	}

	struct block_header *block = (struct block_header *)ptr - 1;
	size_t current_size = block->size;

	// check if the pointer can be realloc'ed
	if (block->magic != BLOCK_MAGIC)
	{
		fprintf(stderr, "[ERROR]: Invalid pointer\n");
		return NULL;
	}

	// if current block is big enough, return it
	if (current_size > size)
		return ptr;

	// allocate the space and verify if it worked
	void *new_ptr = _malloc(size);
	if (!new_ptr)
	{
		fprintf(stderr, "[ERROR]: _malloc failed!\n");
		return NULL;
	}

	// copy the contents from ptr to new_ptr
	size_t min_size = current_size < size ? current_size : size;
	memcpy(new_ptr, ptr, min_size);

	// free the original memory
	_free(ptr);

	return new_ptr;
}

int main()
{
	printf("=== Testing multi-page allocation ===\n");
	printf("Page size: %d\n\n", getpagesize());

	void *ptrs[100];

	for (int i = 0; i < 100; i++)
	{
		ptrs[i] = _malloc(100);
		if (i % 20 == 0)
		{
			printf("Allocated %d blocks, latest: %p\n", i + 1, ptrs[i]);
		}
	}

	printf("\n✓ Successfully allocated 100 blocks\n");
}
