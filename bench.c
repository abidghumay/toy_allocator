#define _POSIX_C_SOURCE 200809L

#include "toy_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (double)ts.tv_sec +
           (double)ts.tv_nsec / 1e9;
}

static void touch_memory(void *p, size_t n)
{
    if (p == NULL || n == 0)
        return;

    unsigned char *x = p;

    x[0] = 1;
    x[n - 1] = 2;
}

static double run_benchmark(
    void *(*alloc_fn)(size_t),
    void (*free_fn)(void *),
    size_t rounds,
    size_t blocks)
{
    void **ptrs = malloc(blocks * sizeof(void *));
    size_t *sizes = malloc(blocks * sizeof(size_t));

    if (!ptrs || !sizes)
    {
        fprintf(stderr, "Benchmark setup failed\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < blocks; i++)
    {
        sizes[i] = 1 + (i * 37u) % 256u;
        ptrs[i] = NULL;
    }

    double start = now_seconds();

    for (size_t r = 0; r < rounds; r++)
    {
        /* allocations */

        for (size_t i = 0; i < blocks; i++)
        {
            ptrs[i] = alloc_fn(sizes[i]);

            if (ptrs[i] == NULL)
            {
                fprintf(stderr,
                        "Allocation failed "
                        "(round=%zu block=%zu)\n",
                        r,
                        i);

                exit(EXIT_FAILURE);
            }

            touch_memory(ptrs[i], sizes[i]);
        }

        /* frees */

        for (size_t i = blocks; i-- > 0;)
        {
            free_fn(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    double end = now_seconds();

    free(ptrs);
    free(sizes);

    return end - start;
}

/* wrappers */

static void *std_alloc(size_t n)
{
    return malloc(n);
}

static void std_free(void *p)
{
    free(p);
}

static void *toy_alloc(size_t n)
{
    return myalloc(n);
}

static void toy_free(void *p)
{
    myfree(p);
}

int main(void)
{
    const size_t rounds = 5000;
    const size_t blocks = 200;

    const double total_ops =
        (double)rounds *
        (double)blocks *
        2.0; /* alloc + free */

    printf("=====================================\n");
    printf("      ALLOCATOR BENCHMARK\n");
    printf("=====================================\n\n");

    double malloc_time =
        run_benchmark(std_alloc,
                      std_free,
                      rounds,
                      blocks);

    double myalloc_time =
        run_benchmark(toy_alloc,
                      toy_free,
                      rounds,
                      blocks);

    double malloc_ops =
        total_ops / malloc_time;

    double myalloc_ops =
        total_ops / myalloc_time;

    printf("malloc/free\n");
    printf("-----------\n");
    printf("time      : %.6f sec\n", malloc_time);
    printf("ops/sec   : %.2f\n\n", malloc_ops);

    printf("myalloc/myfree\n");
    printf("--------------\n");
    printf("time      : %.6f sec\n", myalloc_time);
    printf("ops/sec   : %.2f\n\n", myalloc_ops);

    if (myalloc_time > malloc_time)
    {
        printf("Result: myalloc is %.2fx slower than malloc\n",
               myalloc_time / malloc_time);
    }
    else
    {
        printf("Result: myalloc is %.2fx faster than malloc\n",
               malloc_time / myalloc_time);
    }

    return 0;
}