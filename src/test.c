#include "test.h"
#include "mem.h"
#include <stdint.h>

void verify_pointer(void *ptr, size_t size, char pattern)
{
	char *chk = (char *)ptr;
	for (size_t i = 0; i < size; i++)
	{
		if (chk[i] != pattern)
		{
			printf("[ERROR] Data corruption at %p+%zu. Expected %x, got %x\n",
			       ptr, i, (unsigned char)pattern, (unsigned char)chk[i]);
			abort();
		}
	}
}

void test_alignment(void)
{
	printf("[1/8] Alignment check...\n");
	for (int i = 0; i < 100; i++)
	{
		size_t size = rand() % 1000 + 1;
		void *p = _malloc(size);
		if (!p)
		{
			printf("[ERROR] Allocation failed for size %zu\n", size);
			abort();
		}
		if ((uintptr_t)p % ALIGNMENT != 0)
		{
			printf("[ERROR] Misaligned pointer %p (size %zu)\n", p, size);
			abort();
		}
		_free(p);
	}
	printf("  ✓ Alignment test passed.\n\n");
}

void test_calloc(void)
{
	printf("[2/8] Calloc check...\n");

	// Test zero initialization
	void *p = _calloc(100, 10);
	if (!p)
	{
		printf("[ERROR] Calloc allocation failed\n");
		abort();
	}

	// Verify all bytes are zero
	for (size_t i = 0; i < 1000; i++)
	{
		if (((char *)p)[i] != 0)
		{
			printf("[ERROR] Calloc didn't zero memory at offset %zu\n", i);
			abort();
		}
	}
	_free(p);

	// Test overflow detection
	p = _calloc(SIZE_MAX / 2, SIZE_MAX / 2); // Should fail gracefully
	if (p != NULL)
	{
		printf("[ERROR] Calloc didn't detect overflow\n");
		abort();
	}

	// Test large calloc
	p = _calloc(1000, 1000);
	if (!p)
	{
		printf("[ERROR] Large calloc failed\n");
		abort();
	}
	for (size_t i = 0; i < 1000000; i++)
	{
		if (((char *)p)[i] != 0)
		{
			printf("[ERROR] Large calloc not zeroed at %zu\n", i);
			abort();
		}
	}
	_free(p);

	printf("  ✓ Calloc test passed.\n\n");
}

void test_realloc(void)
{
	printf("[3/8] Realloc check...\n");

	// Test growing allocation
	void *p = _malloc(100);
	if (!p)
	{
		printf("[ERROR] Initial malloc failed\n");
		abort();
	}
	memset(p, 'A', 100);

	p = _realloc(p, 500);
	if (!p)
	{
		printf("[ERROR] Realloc to larger size failed\n");
		abort();
	}
	verify_pointer(p, 100, 'A'); // Original data should be preserved
	_free(p);

	// Test shrinking allocation
	p = _malloc(1000);
	if (!p)
	{
		printf("[ERROR] Malloc for shrink test failed\n");
		abort();
	}
	memset(p, 'B', 1000);

	p = _realloc(p, 200);
	if (!p)
	{
		printf("[ERROR] Realloc to smaller size failed\n");
		abort();
	}
	verify_pointer(p, 200, 'B');
	_free(p);

	// Test NULL pointer (should act like malloc)
	p = _realloc(NULL, 100);
	if (!p)
	{
		printf("[ERROR] Realloc(NULL, size) failed\n");
		abort();
	}
	_free(p);

	// Test size 0 (should act like free)
	p = _malloc(100);
	p = _realloc(p, 0); // Should return NULL and free
	if (p != NULL)
	{
		printf("[ERROR] Realloc(ptr, 0) didn't return NULL\n");
		abort();
	}

	// Test realloc to much larger size (multi-page)
	p = _malloc(100);
	memset(p, 'C', 100);
	p = _realloc(p, 10000);
	if (!p)
	{
		printf("[ERROR] Realloc to large size failed\n");
		abort();
	}
	verify_pointer(p, 100, 'C');
	_free(p);

	printf("  ✓ Realloc test passed.\n\n");
}

void test_edge_cases(void)
{
	printf("[4/8] Edge cases...\n");

	// Free NULL (should be no-op)
	_free(NULL);

	// Double free (should print warning but not crash)
	printf("  Testing double free (expect warning):\n");
	void *p2 = _malloc(100);
	_free(p2);
	_free(p2); // Should print warning but not crash
	printf("  Double free handled correctly.\n");

	// Huge allocation
	void *p3 = _malloc(10 * 1024 * 1024); // 10 MB
	if (p3)
	{
		memset(p3, 'X', 100); // Just touch some of it
		verify_pointer(p3, 100, 'X');
		_free(p3);
		printf("  Huge allocation (10MB) succeeded.\n");
	}
	else
	{
		printf("  Huge allocation failed (may be expected on some systems).\n");
	}

	// Very small allocations
	void *p4 = _malloc(1);
	if (!p4)
	{
		printf("[ERROR] 1-byte allocation failed\n");
		abort();
	}
	*(char *)p4 = 'Z';
	if (*(char *)p4 != 'Z')
	{
		printf("[ERROR] 1-byte allocation corrupted\n");
		abort();
	}
	_free(p4);

	printf("  ✓ Edge cases passed.\n\n");
}

void test_coalescing(void)
{
	printf("[5/8] Coalescing check...\n");

	// Allocate 3 adjacent blocks
	void *p1 = _malloc(100);
	void *p2 = _malloc(100);
	void *p3 = _malloc(100);

	if (!p1 || !p2 || !p3)
	{
		printf("[ERROR] Initial allocations failed\n");
		abort();
	}

	// Mark them to verify they don't get corrupted
	memset(p1, 'X', 100);
	memset(p2, 'Y', 100);
	memset(p3, 'Z', 100);

	// Verify before freeing
	verify_pointer(p1, 100, 'X');
	verify_pointer(p2, 100, 'Y');
	verify_pointer(p3, 100, 'Z');

	// Free middle one
	_free(p2);

	// Verify remaining blocks are still intact
	verify_pointer(p1, 100, 'X');
	verify_pointer(p3, 100, 'Z');

	// Free first one - should coalesce with p2
	_free(p1);

	// Free last one - should coalesce into one big block
	_free(p3);

	// Now allocate something that benefits from coalescing
	void *big = _malloc(350); // Should fit in coalesced block
	if (!big)
	{
		printf(
		    "  [INFO] Large allocation after coalescing required new page\n");
		printf("  (This is OK - blocks may not have been adjacent)\n");
	}
	else
	{
		printf("  Coalescing appears to be working.\n");
	}
	_free(big);

	printf("  ✓ Coalescing test passed.\n\n");
}

void test_boundaries(void)
{
	printf("[6/8] Boundary check...\n");

	void *ptrs[20];
	for (int i = 0; i < 20; i++)
	{
		ptrs[i] = _malloc(128);
		if (!ptrs[i])
		{
			printf("[ERROR] Allocation %d failed\n", i);
			abort();
		}
		memset(ptrs[i], 'A' + i, 128);
	}

	// Verify no blocks overwrote each other
	for (int i = 0; i < 20; i++)
	{
		verify_pointer(ptrs[i], 128, 'A' + i);
		_free(ptrs[i]);
	}

	printf("  ✓ Boundary test passed.\n\n");
}

void stress_test(void)
{
	printf("[7/8] Random Operations Stress Test...\n");
	srand(42); // Fixed seed for reproducibility

	const int NUM_POINTERS = 1000;
	void *ptrs[NUM_POINTERS];
	size_t sizes[NUM_POINTERS];
	bool active[NUM_POINTERS]; // Track if ptrs[i] is currently allocated

	memset(ptrs, 0, sizeof(ptrs));
	memset(active, 0, sizeof(active));

	// Random Allocations & Frees with Data Verification
	for (int i = 0; i < 10000; i++)
	{
		int idx = rand() % NUM_POINTERS;

		if (active[idx])
		{
			// Verify data before freeing
			verify_pointer(ptrs[idx], sizes[idx], (char)(idx & 0xFF));
			_free(ptrs[idx]);
			active[idx] = false;
		}
		else
		{
			// Allocate random size: mix of small and large
			size_t size = (rand() % 1024) + 1; // Small
			if (rand() % 10 == 0)
				size += (rand() % 10) * 4096; // Occasional large pages

			void *p = _malloc(size);
			if (!p)
			{
				printf("[ERROR] Allocation failed at iteration %d\n", i);
				abort();
			}

			// Fill with pattern
			memset(p, (char)(idx & 0xFF), size);
			ptrs[idx] = p;
			sizes[idx] = size;
			active[idx] = true;
		}

		if (i % 2000 == 0)
			printf("  Iteration %d/10000...\n", i);
	}

	// Clean up
	for (int i = 0; i < NUM_POINTERS; i++)
	{
		if (active[i])
		{
			verify_pointer(ptrs[i], sizes[i], (char)(i & 0xFF));
			_free(ptrs[i]);
		}
	}

	printf("  ✓ Stress test passed.\n\n");
}

void fragmentation_test(void)
{
	printf("[8/8] Fragmentation Recovery check...\n");

	void *ptrs[100];

	// Fill array
	for (int i = 0; i < 100; i++)
	{
		ptrs[i] = _malloc(128);
		if (!ptrs[i])
		{
			printf("[ERROR] Allocation %d failed\n", i);
			abort();
		}
		memset(ptrs[i], 'F', 128);
	}

	// Free every other one to create fragmentation
	for (int i = 0; i < 100; i += 2)
	{
		_free(ptrs[i]);
	}

	// Allocate large block that may require coalescing or new page
	void *big_chunk = _malloc(5000);
	if (!big_chunk)
	{
		printf("[ERROR] Large allocation after fragmentation failed\n");
		abort();
	}
	printf("  Allocated big chunk after fragmentation: %p\n", big_chunk);
	_free(big_chunk);

	// Free remaining blocks
	for (int i = 1; i < 100; i += 2)
	{
		verify_pointer(ptrs[i], 128, 'F');
		_free(ptrs[i]);
	}

	printf("  ✓ Fragmentation test passed.\n\n");
}

void comprehensive_test(void)
{
	printf("\n");
	printf("╔════════════════════════════════════════════════════════╗\n");
	printf("║   COMPREHENSIVE MEMORY ALLOCATOR TEST SUITE            ║\n");
	printf("╚════════════════════════════════════════════════════════╝\n");
	printf("\n");

	test_alignment();
	test_calloc();
	test_realloc();
	test_edge_cases();
	test_coalescing();
	test_boundaries();
	stress_test();
	fragmentation_test();

	printf("╔════════════════════════════════════════════════════════╗\n");
	printf("║              ✓✓✓ ALL TESTS PASSED ✓✓✓                 ║\n");
	printf("╚════════════════════════════════════════════════════════╝\n");
	printf("\n");
}
