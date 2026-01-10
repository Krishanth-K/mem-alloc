## Day 1 - 2026-01-9

### Progress
- Started writing the malloc function.
- Used mmap to create some space in virtual memory.

### Task
Tomorrow i have to understand how to "mark" a portion of the virtual memory as "occupied", so other memory blocks
dont share it

### Notes

#### Problem
- Using mmap for a single malloc call is inefficient.
- Cause the os returns a page (4 kb), for any size of data.

#### Solution
- Request a single page from OS, and manage it manually.


## Day 2 - 2026-01-10

