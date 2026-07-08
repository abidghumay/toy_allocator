#include<stdio.h>
#include<unistd.h>   // needed for sbrk()
#include<stddef.h>   // gives size_t,max_align_t = (max alignment of the system usually 16 byte)
#include<stdalign.h>  // gives alignof(T)=(how many byte alignment does T needs) and alignment related tools
#include<stdint.h> // usefull for fixed length int like uint32_t,uint8_t. not used here
#define heap_size (1024*1024)
#define ALIGNMENT alignof(max_align_t) 
#define ALIGN_UP(n) (((n)+(ALIGNMENT-1)) & ~(ALIGNMENT-1))  // rounds up to the next alignment boundary, works because alignment is power of 2
void* heap_start= NULL;
void* heap_end = NULL;
void* movingptr=NULL; // points to header

typedef union {
    struct{
        size_t size;  // size of the payload, lowest bit is used as a flag to indicate if the block is allocated or not
    };// anonymous struct
    max_align_t _align; // ensures that the header is aligned to the maximum alignment requirement of the system
}header_u;
#define HEADER_SIZE sizeof(header_u)
static int allocator_init(void){     // static means only this file can use this as its a helper function
    if(heap_start!=NULL) return 1;  // 1 for successful allocation
    header_u* p=sbrk(heap_size);
    if(p==(void*)-1) return 0;  //on failure sbrk() returns (void*)-1, its a special error value not a memory address

    heap_start=p;
    heap_end=(char*)p+heap_size;
    movingptr= heap_start;
    return 1;
}

 static header_u* find_freed_blocks(size_t n){
    header_u* cursor=heap_start;
    if(movingptr==cursor) return NULL; 
    while(cursor!=movingptr){             // until find an eligible freed block
         size_t real_size= cursor->size &~1;
         size_t is_allocated= cursor->size & 1;
         if(!is_allocated && real_size>=n ){
            if(real_size-n >= ALIGNMENT + HEADER_SIZE ){   // block splitting, checking if the leftover block is big enough to hold a header and at least one alignment unit of payload 
                header_u* leftover=(header_u*)((char *)cursor + n + HEADER_SIZE); // leftover block header
                leftover->size=(real_size-n-HEADER_SIZE)&~1;  // leftover block size, flag cleared
                cursor->size=n|1; // this is the allocated block, flag set
                return cursor;
            }
            cursor->size|=1;            // flagged as allocated
            return cursor;
         }
         else{
            cursor =(header_u*)((char*)cursor + real_size+ HEADER_SIZE); // next header 
         }
    }
    return NULL;
}

void * myalloc(size_t n){
    if(n==0) return NULL;
    if(!allocator_init()) return NULL;  // if heap is not initialized, initialize it
    n= ALIGN_UP(n);  // round up to the next alignment boundary
    header_u * block= find_freed_blocks(n);
    if(block !=NULL){
        return (header_u*) block +1;  // returning payload
    }
    header_u * header= (header_u*)movingptr;
    void* loadptr = (header_u*)movingptr +1;   // points to the payload
    if((char*)loadptr + n > (char*)heap_end) return NULL;
    void* payload= (header_u*)movingptr+ 1;  // start of the allocated payload
    header->size=n|1;// flag set(allocated)
    
    movingptr=(char*)loadptr +n;
    
    
    return payload;
}
 void myfree(void* payload){
    if(payload==NULL) return;
    header_u* head= (header_u*)payload-1;
    size_t real_size= head->size &~1;// read the actual size, masking out the lowest bit
    size_t is_allocated= head->size & 1;// read the flag. 1 is allocated, 0 is not
    if(!is_allocated){
        return ;
    }
    head->size= real_size;  
    while(1){  //  forward coalescing adjacent free blocks
    header_u* next_header= (header_u*)((char*)head + real_size + HEADER_SIZE);
    if((char*)next_header>=(char*)movingptr) break;  /// if next header is beyond the moving pointer, we are done
     if(next_header->size &1) break;   // if next header is allocated, we are done
     
        size_t real_size_next=  next_header->size &~1;
        real_size+=real_size_next+HEADER_SIZE;
        head->size= real_size;
     
    }
 }
