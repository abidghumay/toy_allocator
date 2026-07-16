#ifndef TOY_ALLOCATOR_H
#define TOY_ALLOCATOR_H

#include<stddef.h>
void *myalloc(size_t n);
void myfree(void *p);
void *myrealloc(void *p, size_t n);
void validate_heap(void);
#endif
