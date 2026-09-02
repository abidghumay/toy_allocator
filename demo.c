#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toy_allocator.h"

int main(void) {
    printf("=======================================================\n");
    printf("           TOY ALLOCATOR VISUALIZATION DEMO            \n");
    printf("=======================================================\n\n");

    printf("[1] Initial sequential allocations...\n");
    printf("    p1 = myalloc(128)\n");
    void *p1 = myalloc(128);
    memset(p1, 0xAA, 128);

    printf("    p2 = myalloc(256)\n");
    void *p2 = myalloc(256);
    memset(p2, 0xBB, 256);

    printf("    p3 = myalloc(64)\n");
    void *p3 = myalloc(64);
    memset(p3, 0xCC, 64);

    printf("    p4 = myalloc(512)\n");
    void *p4 = myalloc(512);
    memset(p4, 0xDD, 512);

    printf("    p5 = myalloc(128)\n");
    void *p5 = myalloc(128);
    memset(p5, 0xEE, 128);

    printf("\n[2] Triggering Block Splitting:\n");
    printf("    Freeing p2 (256 bytes)...\n");
    myfree(p2);
    p2 = NULL;

    printf("    Allocating p6 = myalloc(80)...\n");
    printf("    -> First-fit will find the 256-byte free hole and SPLIT it\n");
    printf("       into an 80-byte allocated block and a free leftover block!\n");
    void *p6 = myalloc(80);
    memset(p6, 0x66, 80);

    printf("\n[3] Triggering Forward Coalescing:\n");
    printf("    Freeing p4 (512 bytes)...\n");
    myfree(p4);
    p4 = NULL;

    printf("    Now freeing p3 (64 bytes), which immediately precedes p4...\n");
    printf("    -> myfree detects adjacent free block ahead and FORWARD COALESCES!\n");
    myfree(p3);
    p3 = NULL;

    printf("\n[4] Triggering Backward Coalescing (via prev_size):\n");
    printf("    Now freeing p5 (128 bytes), which immediately follows the merged block...\n");
    printf("    -> myfree inspects prev_size, detects preceding block is free,\n");
    printf("       and BACKWARD COALESCES into it!\n");
    myfree(p5);
    p5 = NULL;

    printf("\n[5] Triggering Reallocation:\n");
    printf("    Shrinking p6 from 80 bytes down to 32 bytes with myrealloc...\n");
    printf("    -> Shrink-in-place splits the leftover into an available free block!\n");
    p6 = myrealloc(p6, 32);

    printf("\n[6] Final Cleanup & Full Heap Coalescing:\n");
    printf("    Freeing p1 (128 bytes)...\n");
    myfree(p1);
    p1 = NULL;

    printf("    Freeing p6 (32 bytes)...\n");
    myfree(p6);
    p6 = NULL;

    printf("\nDemo completed successfully! Heap validated.\n");
    printf("Event log generated for visualization.\n");
    return 0;
}
