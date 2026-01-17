# Memory Allocator - Performance Tracking

## Current Implementation Status

### Features
- ✅ Basic `malloc`, `free`, `calloc`, `realloc`
- ✅ Multi-page allocation support (allocates enough pages for large requests)
- ✅ Coalescing of adjacent free blocks
- ✅ First-fit allocation strategy
- ✅ Magic number validation for memory safety
- ✅ Double-free detection
- ✅ 8-byte alignment

### Architecture
- **Strategy**: First-fit with implicit free list
- **Data Structure**: Doubly-linked list of ALL blocks (both free and allocated)
- **Metadata Size**: 40 bytes per block
  - `size_t size` (8 bytes)
  - `bool is_free` (1 byte + padding)
  - `size_t magic` (8 bytes)
  - `struct block_header *prev` (8 bytes)
  - `struct block_header *next` (8 bytes)
- **Page Management**: Dynamic via `mmap`, allocates multiple pages as needed
- **Coalescing**: On `free()`, merges with adjacent free blocks

### Known Limitations
- All blocks in single linked list (free AND allocated)
- Linear search through entire list for allocation
- No size-class segregation
- No caching of recently freed blocks
- Large metadata overhead (40 bytes per block)
- `validate_list()` calls in hot paths (debug overhead)

---

## Benchmark Results

### Version 1.0 - Baseline (First-Fit, Single List)
**Date**: [Add current date]

| Benchmark | Custom (ms) | System (ms) | Ratio |
|-----------|-------------|-------------|-------|
| Sequential Small Allocs (10k × 64B) | 457.04 | 2.88 | 158.68x ✗ |
| Random Ops (100k ops) | 7.05 | 0.38 | 18.72x ✗ |
| Alloc-Fill-Free (10k × 1KB) | 3.35 | 0.23 | 14.68x ✗ |
| Large Allocations (100 × 8KB) | 3.56 | 0.20 | 17.54x ✗ |
| Fragmentation Test | 4.77 | 0.08 | 57.42x ✗ |
| Realloc Operations (1k ops) | 0.08 | 0.30 | 0.28x ✓ |
| Mixed Workload (100k ops) | 25.38 | 5.13 | 4.95x ✗ |
| **TOTAL** | **502.22** | **9.20** | **54.61x** |

**Analysis**:
- ⚠️ Significantly slower than system malloc (54.61x)
- Sequential small allocations are the worst case (158.68x) - list traversal kills performance
- Realloc is surprisingly good (0.28x) - likely due to test pattern
- Main bottleneck: Linear search through ALL blocks for every allocation

**Next Steps**:
1. Separate free list (only traverse free blocks)
2. Remove `validate_list()` from production code
3. Consider size-class segregation for small allocations

---

## Future Optimizations Plan

### Phase 1: Explicit Free List
- [ ] Maintain separate list of only free blocks
- [ ] Expected improvement: 5-10x on small allocations
- [ ] Tradeoff: Slightly more complex pointer management

### Phase 2: Remove Debug Overhead
- [ ] Remove `validate_list()` calls from hot paths
- [ ] Add compile-time debug flag
- [ ] Expected improvement: 2-3x across the board

### Phase 3: Size-Class Segregation
- [ ] Separate free lists for common sizes (16, 32, 64, 128, 256, 512, 1024 bytes)
- [ ] Quick bins for tiny allocations
- [ ] Expected improvement: 10-20x on sequential small allocations

### Phase 4: Reduce Metadata
- [ ] Explore boundary tags or footer optimization
- [ ] Compress flags into size field
- [ ] Target: 16-24 bytes per block instead of 40

### Phase 5: Advanced Optimizations
- [ ] Thread-local caches (if going multi-threaded)
- [ ] Delayed coalescing
- [ ] Best-fit for certain size classes

---

## Testing

### Test Coverage
- ✅ Alignment verification
- ✅ Calloc zero-initialization
- ✅ Realloc data preservation
- ✅ Multi-page allocation
- ✅ Fragmentation and coalescing
- ✅ Edge cases (NULL, double-free, huge allocs)
- ✅ Boundary checking
- ✅ Stress testing (10k random ops)

### Test Results
All tests passing ✓

---

## Build & Run

```bash
# Compile allocator
cc -O2 -c mem.c -o mem.o

# Run tests
cc -g tests.c mem.o -o tests
./tests

# Run benchmarks
cc -O2 benchmark.c mem.o -o benchmark
./benchmark
```

---

## Notes

### Why is it so slow?
The current implementation traverses EVERY block (free and allocated) on every malloc call. With 10,000 allocations:
- First allocation: checks 1 block
- Second allocation: checks 2 blocks
- ...
- 10,000th allocation: checks 10,000 blocks
- Total: ~50 million block checks!

System `malloc` uses:
- Separate free lists by size class
- Constant-time lookup for common sizes
- Heavily optimized assembly
- Thread-local caches
- Bitmap-based metadata

This is a learning project - being 50x slower is totally normal and expected! The goal is to understand the tradeoffs and optimization strategies.
