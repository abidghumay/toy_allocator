# myalloc

A dynamic memory allocator written from scratch in C, implementing malloc and free without using the standard library versions. Built as a learning project to understand what actually happens inside memory allocators.

## What it does

- Requests a 1MB heap from the OS using sbrk() on first call
- Allocates aligned memory blocks using an implicit free list
- Block splitting — oversized free blocks are split so the remainder stays usable
- Forward coalescing — freeing a block merges it with all consecutive free blocks ahead of it
- Backward coalescing — freeing a block merges it with the free block behind it using a prev_size field in the header, eliminating the need for a separate footer
- Returns payloads aligned to alignof(max_align_t) (16 bytes on 64-bit systems)
- Guards against double-free and null-free

## Memory layout

Each allocation lays out in memory like this:

[ header (16 bytes) ][ payload (n bytes, aligned) ]
The header stores two fields:

typedef union {
    struct {
        size_t size;      // payload size; lowest bit = allocated flag
        size_t prev_size; // previous block's size; lowest bit = prev allocated flag
    };
    max_align_t _align;   // pads header to alignment boundary
} header_u;
Packing the allocated flag into the lowest bit of size works because all sizes are multiples of 16 — the lower bits are always zero and available for free use.

prev_size enables backward coalescing without a footer: to jump to the previous block's header, subtract prev_size + sizeof(header) from the current header's address.

## How the key operations work

myalloc(n):
1. Round n up to the nearest alignment boundary
2. Walk the implicit free list for a block that fits (find_freed_blocks)
3. If found — split it if the leftover is large enough, mark allocated, return payload
4. If not found — extend into fresh heap via bump pointer, write header, return payload

myfree(ptr):
1. Walk back one header from the payload pointer
2. Clear the allocated flag
3. Forward coalesce: loop merging with consecutive free blocks ahead
4. Backward coalesce: check prev_size flag; if previous block is free, merge into it
5. Update the next block's prev_size to reflect the new merged size

## Building

gcc -o myalloc myalloc.c
## Limitations

This is a learning project, not production code:

- No thread safety — concurrent calls will corrupt the heap
- First-fit search — slow on large heaps; real allocators use size-class free lists
- **No realloc / calloc**
- **sbrk()-based** — deprecated on modern Linux; large allocations should use mmap
- **No heap integrity checking** in release builds

## Why I built this

I wanted to understand what actually happens inside malloc — how the OS hands memory to userspace via sbrk, how headers encode metadata with bit packing, how free lists are walked, why alignment matters, and how coalescing prevents fragmentation. Reading about it is one thing; implementing it from scratch and watching gdb reveal the heap layout block by block is another.