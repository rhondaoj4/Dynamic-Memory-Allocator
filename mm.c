//Rhonda Ojongmboh

/*-------------------------------------------------------------------
 *
 *
 * Project stage 1:
 *        A malloc - modify the code to only request more memory
 *            if we don't have free blocks that can fit the request.
 *            We need to free! And keep track of the memory blocks.
 *            Implement a data structure on each block that allows you
 *            to traverse the list. Search free blocks. And be more
 *            efficient. Don't split or coalesce blocks!
 *
 * Project stage 2:
 *        A more efficient malloc - modify the code to split blocks
 *            when overallocation is imminent. Don't forget to write
 *            new metadata, and to update existing metadata to match
 *            the new state of the linked list. When freeing, coalesce
 *            blocks. If freeing creates a sequence of 2 or 3 free
 *            blocks, merge them into a single one; again, modify all the
 *            relevant metadata to make the linked list consistent
 *
 * Project stage 3:
 *        A faster malloc - modify the code to keep track of a second
 *            linked-list composed of free blocks. To do that, we take
 *            some of the block space reserved for program allocations
 *            but ONLY when the block is free! This is a more traditional
 *            linked list where nodes can be in any order, and thus
 *            requires an explicit node.
 *
 *-------------------------------------------------------------------- */

// C libraries for C things
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
// This file provides the memory management functions we need for our implementation
#include "memlib.h"
// This file just contains some function declarations
#include "mm.h"

/**
 * This is a hard-coded heap, use it to debug the initial set of functions before starting the malloc implementation
 * It contains 3 blocks:
 *   - 64B free block
 *   - 32B allocated block
 *   - 32B free block
 */
static long fake_heap[22] = {
    -64, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    32, (unsigned long)(&fake_heap), 1, 1, 1, 1,
    -32, (unsigned long)(&fake_heap[10]), 2, 2, 2, 2
};


/* Macros for unscaled pointer arithmetic to keep other code cleaner.
     Casting to a char* has the effect that pointer arithmetic happens at
     the byte granularity (i.e. POINTER_ADD(0x1, 1) would be 0x2).    (By
     default, incrementing a pointer in C has the effect of incrementing
     it by the size of the type to which it points (e.g. Block).)
     We cast the result to void* to force you to cast back to the
     appropriate type and ensure you don't accidentally use the resulting
     pointer as a char* implicitly. Also avoids compiler complaints.
*/
#define UNSCALED_POINTER_ADD(p, x) ((void *)((char *)(p) + (x)))
#define UNSCALED_POINTER_SUB(p, x) ((void *)((char *)(p) - (x)))


typedef struct _BlockInfo
{
    long int size;          // Size of block
    struct _Block *prev;    // Explicit pointer to previous Block
} BlockInfo_t;

typedef struct _FreeBlockInfo
{
    struct _Block *nextFree;    // Explicit pointer to next free Block (stage 3)
    struct _Block *prevFree;    // Explicit pointer to previous free Block (stage 3)
} FreeBlockInfo_t;

typedef struct _Block
{
    BlockInfo_t info;           // Composing both infos into a single struct
    FreeBlockInfo_t freeNode;   //  Think: What does this mean in terms of memory?
} Block_t;

typedef struct
{
    Block_t *malloc_list_tail;  // Use keep track of the tail node
    Block_t *free_list_head;    // Pointer to the first FreeBlockInfo_t in the free list, use in stage 3.
} malloc_info_t;

/* Variable to keep malloc information tidy */
static malloc_info_t malloc_info = {
    .free_list_head = NULL,
    .malloc_list_tail = NULL,
};

/* Alignment of blocks returned by mm_malloc.
 * (We need each allocation to at least be big enough for the free space
 * metadata... so let's just align by that.)    */
#define ALIGNMENT (sizeof(FreeBlockInfo_t))


/************************************************************************
 * Suggested memory management/navigation functions for the project.
 *   Start by implementing these, they will make your life easier and
 *   force you to thing about how the memory navigation/management works.
 ************************************************************************/

// Declarations
Block_t *first_block();
Block_t *next_block(Block_t *block);
void *request_more_space(size_t request_size);
size_t heap_size();

/**
 * This function should get the first block or returns NULL if there is not one.
 * You can use this to start your through search for a block.
 */
Block_t *first_block()
{

    if(heap_size() == 0){
        return NULL;
    }
    Block_t *first = (Block_t*)(mem_heap_lo());
    
    return first;
}

/**
 * This function will get the adjacent block or returns NULL if there is not one.
 * You can use this to move along your malloc list one block at a time.
 */
Block_t *next_block(Block_t *block)
{

    //return if block is null
    if(block == NULL){
        return NULL;
    }

    //absolute value of data size
    int x = abs(block->info.size);
    
    //next = base address + metadata + size of the data
    Block_t *next = (Block_t*)UNSCALED_POINTER_ADD((block), (sizeof(BlockInfo_t) + x));
	
	if ((char*)next >= (char*)mem_heap_hi()){

		//printf("END OF HEAP");
		return NULL;
	}
	
    return next;
}

/* This function will have the OS allocate more space for our heap.
 *
 * It returns a pointer to that new space. That pointer will always be
 * larger than the last request and be continuous in memory.
 */
void *request_more_space(size_t request_size)
{    
    //increase heap size by the requested amount and point at new heap space
    void *ret = (Block_t*)(mem_sbrk(request_size)); 

    //return type of a failed call to mem_sbrk
    if (ret == (void*)-1)
    {
        // Let the program crash if you run out of memory!
        printf("ERROR: failed to request_more_space\n");
        exit(0);
    }
    return ret;
}

/* Returns the size of the heap */
size_t heap_size()
{
    size_t size = mem_heapsize();
    return size;
}

/************************************************************************
 * Suggested heap management/navigation functions for the project.
 *   Start by implementing these, they will make your life easier and
 *   force you to thing about how the memory navigation/management works.
 ************************************************************************/

/******************************* Stage 1 ********************************/
/* Find a free block of at least the requested size in the heap.
    Returns NULL if no free block is large enough. */
Block_t *search_list(size_t request_size)
{
    // ptr_free_block will point to the beginning of the memory heap!
    Block_t *ptr_free_block = first_block();

    //if ptr_free_block is still traversing the heap
    while(ptr_free_block != NULL){

        //size of ptr_free_block data
        long int dataSize = ptr_free_block->info.size;
        //absolute value of size of data
        long int absSize = abs(dataSize);

        //if we have a free block && request_size < the size of the free block
        if(dataSize < 0 && (request_size <= absSize)){

            //we have found a free block with enough space
            return ptr_free_block;
        }

        //move to the next block, since we have not found a free block
        ptr_free_block = next_block(ptr_free_block);

    }

    return NULL;


}

/******************************* Stage 2 ********************************/
/* Shrink block to size, and create a new block with remaining space. */
void split(Block_t *block, size_t size) {

	//full length of original block
	//assert(block->info.size < 0);
	long int ogSize = block->info.size;

    //remove the original big block
    remove_free_node(block);

	//shrunken block
	block->info.size = size;

	//new block allocated
	Block_t* newBlock = (Block_t*)UNSCALED_POINTER_ADD(block, (size + sizeof(BlockInfo_t)));

	//new block info.size is being set
	newBlock->info.size = -(abs(ogSize) - sizeof(BlockInfo_t) - size);
    insert_free_node(newBlock);

	//ADJUSTING THE POINTERS

	newBlock->info.prev = block;

	//someBlock is the block that was after the original block
	Block_t* someBlock = next_block(newBlock);


	if(someBlock != NULL){

	//someBlock.prev now points to newBlock
	someBlock->info.prev = newBlock;
	}
	else{
	    //newBlock is the last block in our list
	    malloc_info.malloc_list_tail = newBlock;
	}
	//assert(next_block(block) == newBlock || next_block(block) == NULL);
	    

}

/* Merge together consecutive free blocks. */
void coalesce(Block_t *block) {


    //how much free data we have
    long int freeData = abs(block->info.size);

    Block_t *prevBlock = block->info.prev;
    Block_t *nextBlock = next_block(block);

    //block is the only element in the list
    // if(nextBlock == NULL && prevBlock == NULL){
    //     return;
    // }

    //the adjacent blocks are occupied or NULL
    if((!nextBlock || nextBlock->info.size > 0 ) && (!prevBlock || prevBlock->info.size > 0)){
        return;
    }    

    //if there is a nextBlock and it's free
    if(nextBlock && nextBlock->info.size < 0){

        
        remove_free_node(nextBlock);
        remove_free_node(block);

        //add nextBlock's info to freeData
        freeData += (abs(nextBlock->info.size) + sizeof(BlockInfo_t));

        //if there is a block after nextBlock, we must point its prev to block
        if(next_block(nextBlock)){

            Block_t * nextNextBlock = next_block(nextBlock);
            nextNextBlock->info.prev = block;

        }
        else{
            //otherwise nextBlock was the last block, and now that we merge, block
            //becomes the last block
            malloc_info.malloc_list_tail = block;
        }


        block->info.size = -freeData;

        //bigger freeNode made up of nextBlock and block
        insert_free_node(block);

    }

    //if there is a previous block and it's free
    if(prevBlock && prevBlock->info.size < 0){

        remove_free_node(block);
        remove_free_node(prevBlock);

        //add prevBlock's info to freeData
        freeData += (abs(prevBlock->info.size) + sizeof(BlockInfo_t));

        //block now is located where prevBlock was
        block = prevBlock;
        block->info.size = -freeData;

        if(next_block(block)){
           Block_t* nextNextBlock = next_block(block);
           nextNextBlock->info.prev = block;
        }
        else{
            malloc_info.malloc_list_tail = block;
        }

        insert_free_node(block);
 

    }

    
    

}

/******************************* Stage 3 ********************************/
/* Insert free block into the free_list.
 */
void insert_free_node(Block_t *block){

    if(block->info.size > 0){
        return;
    }

    //INSERTING AT THE FRONT


    //block is the first element in the free list
    if(!malloc_info.free_list_head){
        malloc_info.free_list_head = block;
        block->freeNode.nextFree = NULL;
        block->freeNode.prevFree = NULL;
        return;
    }


    //not the first element in the list
    if(malloc_info.free_list_head){
    
        //temp = first block in list
       Block_t* temp = malloc_info.free_list_head;

       //first block in list changed to block
       malloc_info.free_list_head = block;

       //next free Block is temp
       block->freeNode.nextFree = temp;
       //there's no block before the first free
       block->freeNode.prevFree = NULL;
       //block before temp is the new firstNode
       temp->freeNode.prevFree = block;
       
    }

}

/* Remove free block from the free_list.
 */
void remove_free_node(Block_t *block) {

    //what do we do if block is the only block
    if(malloc_info.free_list_head == block && block->freeNode.nextFree == NULL){

        malloc_info.free_list_head = NULL;
        return;
    }

    //what do we do if it's the first block
    if(malloc_info.free_list_head == block){

        //head of malloc is now the block that was after block
        malloc_info.free_list_head = block->freeNode.nextFree;
        //new head.prev is set to NULL
        malloc_info.free_list_head->freeNode.prevFree = NULL;
        
        return;
        
    }
    //367 causing me issues

    //what do we do if block is the last block
    if(block->freeNode.nextFree == NULL){

        Block_t* prevBlock = block->freeNode.prevFree;
        prevBlock->freeNode.nextFree = NULL;

        //block->freeNode.prevFree->freeNode.nextFree == NULL;

        return;
    }

    //if its in the middle

    Block_t *prevBlock = block->freeNode.prevFree;
    Block_t *nextBlock = block->freeNode.nextFree;

    prevBlock->freeNode.nextFree = nextBlock;
    nextBlock->freeNode.prevFree = prevBlock;


    block->freeNode.prevFree = NULL;
    block->freeNode.nextFree = NULL;

    

}

/* Find a free block of at least the requested size in the free list.
    Returns NULL if no free block is large enough. */
Block_t *search_free_list(size_t request_size)
{
    Block_t *ptr_free_block = malloc_info.free_list_head;
    long int check_size = -request_size;


    //while we are still travering our free list
    while(ptr_free_block){

        //used 'less than' to compare negatives
        if(ptr_free_block->info.size <= check_size){
            return ptr_free_block;
        }
        //go to the next block in the freeNode
        ptr_free_block = ptr_free_block->freeNode.nextFree;

    }

    return NULL;
}

// TOP-LEVEL ALLOCATOR INTERFACE ------------------------------------

/* Initialize the allocator. */
int mm_init()
{
    // Modify this function only if you add variables that need to be initialized.
    // This will be called ONCE at the beginning of execution
    malloc_info.free_list_head = NULL;
    malloc_info.malloc_list_tail = NULL;


    return 0;
}

/* Allocate a block of size size and return a pointer to it. If size is zero,
 * returns null.
 */
void *mm_malloc(size_t size)
{
    Block_t *ptr_free_block = NULL;

    // Zero-size requests get NULL.
    if (size == 0){
        return NULL;
    }

    //examine_heap();
    // Determine the amount of memory we want to allocate
    // Round up for correct alignment
    long int request_size = ALIGNMENT * ((size + ALIGNMENT - 1) / ALIGNMENT);

     //ptr_free_block = search_list(request_size);
    ptr_free_block = search_free_list(request_size);

    if(ptr_free_block){
    
        if (abs(ptr_free_block->info.size) > request_size + sizeof(BlockInfo_t)){
            split(ptr_free_block, request_size);
        }
        else{
            remove_free_node(ptr_free_block);
            ptr_free_block->info.size = -ptr_free_block->info.size;
            
        }
        return (Block_t*)UNSCALED_POINTER_ADD(ptr_free_block, sizeof(BlockInfo_t));
    }



    //EXTENDING THE SIZE OF THE HEAP
    if(ptr_free_block==NULL){
            
    //we didn't find a block, we ask for the size of a block
    ptr_free_block = (Block_t*)(request_more_space(request_size + sizeof(BlockInfo_t)));

    //setting the size of the new block
    ptr_free_block->info.size = request_size;
    
    //REMEMBER TO MOVE THE TAIL TO THE NEW TAIL SINCE WE CHANGED THE SIZE OF THE HEAP
    if(malloc_info.malloc_list_tail != NULL){

        //ptr.prev = old last block
        ptr_free_block->info.prev = malloc_info.malloc_list_tail;
    }
    else{
        //first block in heap
        ptr_free_block->info.prev = NULL;
    }

    //ptr_free_block becomes the new tail
    malloc_info.malloc_list_tail = ptr_free_block;

    // base + metadata to enter the user accessable data
    return (Block_t*)UNSCALED_POINTER_ADD(ptr_free_block, sizeof(BlockInfo_t));

    }
    
    // This funciton will cost you performance, remove it if you are not debugging.
    // #warning This is a reminder, don't use these function after you are done debugging!
    // {
    //     // This function prints the heap
    //     examine_heap();
    //     // This function checks some common issues with the heap
        check_heap();
    // }

    return NULL;


}

    

/* Free the block referenced by ptr. */
void mm_free(void *ptr)
{
    //fprintf(stderr, "%p\n", ptr);
    //examine_heap();
    Block_t *block = (Block_t *)UNSCALED_POINTER_SUB(ptr, sizeof(BlockInfo_t));

    if(block == NULL){
        return;
    }

    long int blockSize = block->info.size;


    //checking if it's an occupied block
    if (blockSize > 0){

        //free the block
        blockSize = -blockSize;
    }

    block->freeNode.nextFree = NULL;
    block->freeNode.prevFree = NULL;

    

    block->info.size = blockSize;
    insert_free_node(block);

    //examine_heap();

    // You can change or remove the declarations above.
    // They are included as minor hints.

    // When you are ready... you will want to implement coalescing:
    coalesce(block);
}

/**********************************************************************
 * PROVIDED FUNCTIONS
 *
 * Note: Need to correctly implement functions
 *     first_block and heap_size
 **********************************************************************/

/* Print the heap by iterating through it as an implicit free list. */
void examine_heap()
{
    /* print to stderr so output isn't buffered and not output if we crash */
    Block_t *curr = (Block_t *)first_block();
    Block_t *end = (Block_t *)UNSCALED_POINTER_ADD(first_block(), heap_size());
    fprintf(stderr, "====================================================\n");
    fprintf(stderr, "heap size:\t0x%lx\n", heap_size());
    fprintf(stderr, "heap start:\t%p\n", curr);
    fprintf(stderr, "heap end:\t%p\n", end);

    fprintf(stderr, "free_list_head: %p\n", (void *)malloc_info.free_list_head);

    fprintf(stderr, "malloc_list_tail: %p\n", (void *)malloc_info.malloc_list_tail);

    while (curr && curr < end)
    {
        /* print out common block attributes */
        fprintf(stderr, "%p: %ld\t", (void *)curr, curr->info.size);

        /* and allocated/free specific data */
        if (curr->info.size > 0)
        {
            fprintf(stderr, "ALLOCATED\tprev: %p\n", (void *)curr->info.prev);
        }
        else
        {
            fprintf(stderr, "FREE\tnextFree: %p, prevFree: %p, prev: %p\n", (void *)curr->freeNode.nextFree, (void *)curr->freeNode.prevFree, (void *)curr->info.prev);
        }

        curr = next_block(curr);
    }
    fprintf(stderr, "END OF HEAP\n\n");

    curr = malloc_info.free_list_head;
    fprintf(stderr, "Head ");
    while (curr)
    {
        fprintf(stderr, "-> %p ", curr);
        curr = curr->freeNode.nextFree;
    }
    fprintf(stderr, "\n");
}

/* Checks the heap data structure for consistency. */
int check_heap()
{
    Block_t *curr = (Block_t *)first_block();
    Block_t *end = (Block_t *)UNSCALED_POINTER_ADD(first_block(), heap_size());
    Block_t *last = NULL;
    long int free_count = 0;

    while (curr && curr < end)
    {
        if (curr->info.prev != last)
        {
            fprintf(stderr, "check_heap: Error: previous link not correct.\n");
            examine_heap();
        }

        if (curr->info.size <= 0)
        {
            // Free
            free_count++;
        }

        last = curr;
        curr = next_block(curr);
    }

    curr = malloc_info.free_list_head;
    last = NULL;
    while (curr)
    {
        if (curr == last)
        {
            fprintf(stderr, "check_heap: Error: free list is circular.\n");
            examine_heap();
        }
        last = curr;
        curr = curr->freeNode.nextFree;
        if (free_count == 0)
        {
            fprintf(stderr, "check_heap: Error: free list has more items than expected.\n");
            examine_heap();
        }
        free_count--;
    }

    return 0;
}
