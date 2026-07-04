# myalloc

A minimal dynamic memory allocator written in C from scratch — implementing malloc and free without using the standard library versions.

## What it does

- Requests a 1MB heap from the OS using sbrk() on initialization
- Allocates memory blocks with a header storing size and an allocation flag
- Reuses freed blocks via an implicit free list — walking all blocks to find a free one large enough
- Guards against double-free and null-free
- Returns payloads aligned to alignof(max_align_t) (16 bytes on 64-bit systems)

## How it works

Each allocation lays out in memory like this:

[ header_u (16 bytes) ][ payload (n bytes, aligned) ]
The header stores the block size with the lowest bit used as an allocated flag — a trick that works because all sizes are multiples of 16, so the lower bits are always zero and available.

header->size = n | 1;   // mark allocated
real_size = head->size & ~1;  // extract size
is_allocated = head->size & 1; // extract flag
To free a block, myfree() walks back one header from the payload pointer and clears the flag:

header_u* head = (header_u*)payload - 1;
head->size = head->size & ~1;
To find a reusable block, find_freed_blocks() walks from heap_start to movingptr (the boundary between initialized and fresh heap), jumping block-to-block using each header's size:

cursor = (header_u*)((char*)cursor + real_size + sizeof(header_u));
## What's missing (intentionally)

This is a learning project, not production code. Missing features include:

- Coalescing — adjacent free blocks are not merged, so fragmentation builds up over time
- Block splitting — a 1000-byte free block given to a 16-byte request wastes 984 bytes
- Thread safety — no locks
- **realloc / calloc**

## Building

gcc -o myalloc myalloc.c
## Why I built this

I wanted to understand what actually happens inside malloc — how the OS hands memory to userspace, how headers work, how free lists are walked, and why alignment matters. This is the result of building it from scratch rather than reading about it.
