**CORE (Must have or it's not an allocator):**

1. **malloc(size_t size)** - Allocate `size` bytes, return pointer or NULL on failure
2. **free(void *ptr)** - Deallocate memory at `ptr`
3. **Memory management strategy** - How do you track free vs allocated blocks? Pick one:
   - Free list (linked list of free blocks)
   - Segregated free lists (different lists for different sizes)
   - Bitmap
   - Buddy system
4. **Coalescing** - When you free a block, merge adjacent free blocks or you'll fragment memory to death
5. **Splitting** - When allocating from a large free block, split it and return the leftover to free pool
6. **Alignment** - All returned pointers must be properly aligned (usually 8 or 16 bytes). Unaligned pointers = crashes on many architectures
7. **Metadata storage** - Store block size and allocation status. Where? Header before each block? Separate structure?

**STANDARD (Expected in real allocators):**

8. **calloc(size_t nmemb, size_t size)** - Allocate and zero initialize. Must check for overflow: `nmemb * size`
9. **realloc(void *ptr, size_t size)** - Resize allocation. Can you expand in place? Otherwise malloc+copy+free
10. **Error handling** - Return NULL on failure, set errno appropriately

**PERFORMANCE (Separates garbage from production-grade):**

11. **Size classes** - Small allocations (< 512 bytes) should be FAST. Use segregated lists or bins
12. **Large allocation optimization** - Big requests (> page size) should use `mmap` directly, not your heap
13. **Thread safety** - Mutexes or per-thread arenas. Single-threaded allocators are toys
14. **Minimize syscalls** - Request big chunks from OS, subdivide them yourself. Don't call `mmap` for every malloc

**CORRECTNESS (Or you'll spend days debugging crashes):**

15. **Double-free detection** - freeing the same pointer twice should not corrupt your heap
16. **Invalid free detection** - freeing random pointers should fail gracefully, not explode
17. **Boundary tags** - Store metadata at both ends of blocks for easier coalescing
18. **Canaries/guards** - Detect buffer overflows by putting magic values around blocks

**BONUS (If you want to flex):**

19. **Memory pools** - Pre-allocate for common sizes
20. **Defragmentation** - Compact memory to reduce fragmentation
21. **Statistics/debugging** - Track allocations, leaks, fragmentation
22. **Custom alignment** - Support `aligned_alloc()` or `posix_memalign()`

**START HERE - Minimal viable allocator:**
1. Request one big chunk via `mmap` (say 1MB)
2. Implement a simple free list with first-fit
3. malloc: find free block big enough, mark as used, return pointer
4. free: mark block as free, add to free list
5. NO coalescing, NO splitting, fixed size blocks initially

