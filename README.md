# toy_allocator

A dynamic memory allocator written from scratch in C, implementing `malloc`, `free`, and `realloc` without using the standard library versions. Built as a learning project to understand what actually happens inside memory allocators.

## What it does

- Requests a 1MB heap from the OS using `sbrk()` on first call
- Allocates aligned memory blocks using an **implicit free list**
- **Block splitting** — oversized free blocks are split so the remainder stays usable
- **Forward coalescing** — freeing a block merges it with all consecutive free blocks ahead of it
- **Backward coalescing** — freeing a block merges it with the free block behind it using a `prev_size` field in the header, eliminating the need for a separate footer
- **`myrealloc`** — shrinks in place (with split), grows in place if the next block is free, extends the heap if it's the last block, falls back to alloc+memcpy+free otherwise
- **Heap validation** (`validate_heap`) — walks every block checking size, alignment, and `prev_size` consistency; disabled in release builds via `NDEBUG`
- Returns payloads aligned to `alignof(max_align_t)` (16 bytes on 64-bit systems)
- Guards against double-free and null-free

## Memory layout

Each allocation lays out in memory like this:

```
[ header (16 bytes) ][ payload (n bytes, aligned) ]
```

The header stores two fields:

```c
typedef union {
    struct {
        size_t size;      // payload size; lowest bit = allocated flag
        size_t prev_size; // previous block's size; lowest bit = prev allocated flag
    };
    max_align_t _align;   // pads header to alignment boundary
} header_u;
```

Packing the allocated flag into the lowest bit of `size` works because all sizes are multiples of 16 — the lower bits are always zero and available for free use.

`prev_size` enables backward coalescing without a footer: to jump to the previous block's header, subtract `prev_size + sizeof(header)` from the current header's address.

## How the key operations work

**myalloc(n):**
1. Round `n` up to the nearest alignment boundary
2. Walk the implicit free list for a block that fits (`find_freed_blocks`)
3. If found — split it if the leftover is large enough, mark allocated, return payload
4. If not found — extend into fresh heap via bump pointer, write header, return payload

**myfree(ptr):**
1. Walk back one header from the payload pointer
2. Clear the allocated flag
3. Forward coalesce: loop merging with consecutive free blocks ahead
4. Backward coalesce: check `prev_size` flag; if previous block is free, merge into it
5. Update the next block's `prev_size` to reflect the new merged size

**myrealloc(ptr, new_size):**
1. If `ptr` is NULL — delegate to `myalloc`
2. If `new_size` is 0 — delegate to `myfree`, return NULL
3. Shrink: split off a free leftover block if big enough, return same pointer
4. Grow: try in-place expansion into the next block if it is free; if the block is the last one just extend `movingptr`; otherwise fall back to alloc + memcpy + free

## Building

```bash
# debug build — heap validation enabled
gcc -o allocator toy_allocator.c

# release build — heap validation disabled
gcc -DNDEBUG -O2 -o allocator toy_allocator.c
```

## Testing

Run with validation on to catch heap corruption early:

```bash
gcc -o allocator toy_allocator.c
./allocator
```

## Visualizer

An interactive browser-based visualizer that renders the heap as a strip of proportional blocks with animated transitions for allocations, block splitting, and coalescing.

```bash
# Build and run the showcase demo to generate the event trace
make run-demo

# Start the visualization server
make serve
# Open http://localhost:5000 in your browser
```

The visualizer instrumentation is optional (compile-time flag `-DTOY_VISUALIZE`), ensuring zero overhead when running benchmarks.

## Benchmark

`bench.c` compares `myalloc`/`myfree` against glibc `malloc`/`free` over 5000 rounds of 200 allocations (sizes 1–256 bytes).

```bash
# with heap validation (expect ~1500x slower — measures validator not allocator)
gcc -o bench bench.c toy_allocator.c
./bench

# without heap validation (real comparison)
gcc -DNDEBUG -O2 -o bench bench.c toy_allocator.c
./bench
```

Results on WSL2 / x86-64 (approximate, varies per run):

```
Without validation:
  malloc/free   — 0.007 sec  (~266M ops/sec)
  myalloc/myfree — 0.265 sec  (~7.5M ops/sec)
  myalloc is ~35x slower than malloc

With validation:
  malloc/free   — 0.021 sec  (~94M ops/sec)
  myalloc/myfree — 32.0 sec   (~62K ops/sec)
  myalloc is ~1506x slower than malloc
```

The 35x gap without validation is expected. glibc uses a per-thread cache (tcache) for small allocations that is effectively O(1). This allocator uses first-fit linear search which is O(n) in the number of live blocks.

The 1506x gap with validation is the cost of calling `validate_heap` — an O(n) heap walk — before and after every single allocation and free. This is for correctness testing only, not production use.

## Limitations

This is a learning project, not production code:

- **No thread safety** — concurrent calls will corrupt the heap
- **First-fit search** — slow on large heaps; real allocators use size-class free lists (tcache, jemalloc bins)
- **`sbrk()`-based** — deprecated on modern Linux; large allocations should use `mmap`
- **Single 1MB heap** — not growable beyond that

## Notes & Attribution

- **Core Allocator**: The core allocator logic (`toy_allocator.c` and `toy_allocator.h`) was written completely from scratch in C to deeply understand how memory allocators work under the hood.
- **Visualizer**: The Python visualizer backend and animated browser frontend (`server.py`, `static/`, and logging instrumentation) were created with the help of Gemini.
- **Benchmark**: `bench.c` was written with LLM assistance for the benchmark harness.

## Why I built this

I wanted to understand what actually happens inside `malloc` — how the OS hands memory to userspace via `sbrk`, how headers encode metadata with bit packing, how free lists are walked, why alignment matters, and how coalescing prevents fragmentation. Reading about it is one thing; building it from scratch and watching the heap layout in memory is another.