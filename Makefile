CC ?= gcc
CFLAGS_COMMON = -Wall -Wextra
CFLAGS_RELEASE = -DNDEBUG -O2 $(CFLAGS_COMMON)
CFLAGS_VISUALIZE = -DTOY_VISUALIZE $(CFLAGS_COMMON)
PYTHON ?= python3

.PHONY: all demo bench serve clean run-demo

all: demo bench

# Demo with full visualization instrumentation
demo: demo.c toy_allocator.c toy_logger.c toy_allocator.h toy_logger.h
	$(CC) $(CFLAGS_VISUALIZE) -o demo demo.c toy_allocator.c toy_logger.c

# Run demo to generate allocator_events.jsonl
run-demo: demo
	./demo

# Benchmark without visualization (zero overhead, NDEBUG)
bench: bench.c toy_allocator.c toy_allocator.h
	$(CC) $(CFLAGS_RELEASE) -o bench bench.c toy_allocator.c

# Run benchmark
run-bench: bench
	./bench

# Start the Python visualization server
serve: demo
	$(PYTHON) server.py 5000

clean:
	rm -f demo bench allocator_events.jsonl
