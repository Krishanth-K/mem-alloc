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

void initHeap()
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

	free_list = header;
}

void *_malloc(size_t length)
{
	if (!free_list)
		initHeap();

	// align the length
	length = ALIGN(length);

	if (free_list->size < length)
		printf("TODO: Create another page or smthng\n");

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

	if (!current)
	{
		fprintf(stdout, "No space found");
		return NULL;
	}

	// return pointer to data section (skip the header)
	return (void *)(current + 1);
}

void coalesce(struct block_header *header)
{
	// merge with next header
	if (header->next && header->next->is_free)
	{
		struct block_header *next = header->next;

		header->size += next->size + sizeof(struct block_header);
		header->next = next->next;

		if (header->next)
			header->next->prev = header;
	}

	// merge with prev header
	if (header->prev && header->prev->is_free)
	{
		struct block_header *prev = header->prev;

		prev->size += header->size + sizeof(struct block_header);
		prev->next = header->next;

		if (prev->next)
			prev->next->prev = prev;
	}
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
		fprintf(stderr, "[ERROR]: Invalid pointer");
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
	printf("=== Testing realloc ===\n");

	// Test 1: realloc(NULL, size) should work like malloc
	int *p = (int *)_realloc(NULL, 10 * sizeof(int));
	printf("realloc(NULL, 40) = %p\n", p);

	// Test 2: Fill with data
	for (int i = 0; i < 10; i++)
		p[i] = i;

	// Test 3: Grow - should preserve data
	p = (int *)_realloc(p, 20 * sizeof(int));
	printf("After growing to 80 bytes:\n");
	for (int i = 0; i < 10; i++)
	{
		printf("%d ", p[i]); // Should still be 0-9
	}
	printf("\n");

	// Test 4: Shrink - should still work
	p = (int *)_realloc(p, 5 * sizeof(int));
	printf("After shrinking to 20 bytes:\n");
	for (int i = 0; i < 5; i++)
	{
		printf("%d ", p[i]); // Should be 0-4
	}
	printf("\n");

	// Test 5: realloc to 0 should free
	p = (int *)_realloc(p, 0);
	printf("realloc(p, 0) = %p (should be NULL)\n", p);
}
