/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "bcalabre+cemmel",
    /* First member's full name */
    "Brent Calabresi",
    /* First member's email address */
    "bcalabre@u.rochester.edu",
    /* Second member's full name (leave blank if none) */
    "Clayton Emmel",
    /* Second member's email address (leave blank if none) */
    "cemmel@u.rochester.edu"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*Additional Macros defined*/
#define WSIZE 4                                                                             //Size of a word
#define DSIZE 8                                                                             //Size of a double word
#define CHUNKSIZE 16                                                                        //Initial heap size
#define OVERHEAD 24                                                                         //The minimum block size
#define MAX(x ,y)  ((x) > (y) ? (x) : (y))                                                  //Finds the maximum of two numbers
#define PACK(size, alloc)  ((size) | (alloc))                                               //Put the size and allocated byte into one word
#define GET(p)  (*(size_t *)(p))                                                            //Read the word at address p
#define PUT(p, value)  (*(size_t *)(p) = (value))                                           //Write the word at address p
#define GET_SIZE(p)  (GET(p) & ~0x7)                                                        //Get the size from header/footer
#define GET_ALLOC(p)  (GET(p) & 0x1)                                                        //Get the allocated bit from header/footer
#define HDRP(bp)  ((void *)(bp) - WSIZE)                                                    //Get the address of the header of a block
#define FTRP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)                               //Get the address of the footer of a block
#define NEXT_BLKP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)))                                  //Get the address of the next block
#define PREV_BLKP(bp)  ((void *)(bp) - GET_SIZE(HDRP(bp) - WSIZE))                          //Get the address of the previous block
#define NEXT_FREEP(bp)  (*(void **)(bp + DSIZE))                                            //Get the address of the next free block
#define PREV_FREEP(bp)  (*(void **)(bp))                                                    //Get the address of the previous free block

static char *heap_listp = 0;
static char *free_listp = 0;                                                                

//Helpers function headers
static void *extend_heap(size_t words);
static void  place(void *bp, size_t size);
static void *find_fit(size_t size);
static void *coalesce(void *bp);
static void  insert_at_front(void *bp);
static void  remove_block(void *bp);
static int   check_block(void *bp);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = memsbrk(OVERHEAD * 2)) == NULL) return -1;//Return on heap failure
    
    //Manually in'PUT'ting proper format for the initial heap
    PUT(heap_listp, 0);
    PUT(heap_listp + WSIZE, PACK(OVERHEAD, 1));
    PUT(heap_listp + DSIZE, 0);
    PUT(heap_listp + DSIZE + WSIZE, 0);
    PUT(heap_listp + OVERHEAD, PACK(OVERHEAD, 1));
    PUT(heap_listp + WSIZE + OVERHEAD, PACK(0,1));
    free_listp = DSIZE + heap_listp;

    if(extended_heap(CHUNKSIZE / WSIZE) == NULL) return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














