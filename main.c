#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>

void *_malloc(int length)
{
	void *start = mmap(NULL, length, PROT_WRITE | PROT_READ,
	                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (start == MAP_FAILED)
	{
		perror("Map failed");
		return NULL;
	}

	printf("pointer: %p", start);
	return start;
}

int main() { _malloc(4); }
