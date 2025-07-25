#include <stdio.h>
#include <stddef.h> 
typedef struct _BlockInfo
{
    long int size;
    struct _Block *prev;
} BlockInfo_t;

typedef struct _FreeBlockInfo
{
    struct _Block *nextFree;
    struct _Block *prevFree;
} FreeBlockInfo_t;

typedef struct _Block
{
    BlockInfo_t info;
    FreeBlockInfo_t freeNode;
} Block_t;

extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);


extern Block_t *first_block();
extern Block_t *next_block(Block_t *block);
extern void *request_more_space(size_t request_size);
extern size_t heap_size();
extern Block_t *search_list(size_t request_size); // Stage 1 search
extern void split(Block_t *block, size_t size);
extern void coalesce(Block_t *block);
extern void insert_free_node(Block_t *block);
extern void remove_free_node(Block_t *block);
extern Block_t *search_free_list(size_t request_size); // Stage 3 search

extern void examine_heap();
extern int check_heap();
