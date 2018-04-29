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

/**************************
 * MAIN FUNCTIONS
 **************************/

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
    	size_t adjustedsize;
	size_t extendedsize;
	char* bp;

	if(size <= 0) return NULL;

	adjustedsize = MAX(ALIGN(size) + DSIZE, OVERHEAD);

	if(bp = fint_fit(adjustedsize))
	{
		place(bp, adjustedsize);
		return bp;
	}

	extendedsize = MAX(adjustedsize, CHUNKSIZE);

	if((bp = extended_heap(extendedsize / WSIZE)) == NULL) return NULL;

	place(bp, adjustedsize);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	if(!ptr) return;

	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t prevSize;
    void* ptr2;
    size_t newSize = MAX(ALIGN(size) + DSIZE, OVERHEAD);
     if(size <= 0)
     {
	     mm_free(ptr);
	     return 0;
     }

     if(ptr == NULL)
     {
	     return mm_malloc(size);
     }

     if(prevSize == newSize) return ptr;
     if(newSize <= prevSize)
     {
	size = newSize;
     	if(prevSize - size <= OVERHEAD) return ptr;

    	PUT(HDRP(ptr), PACK(size, 1));
     	PUT(FTRP(ptr), PACK(size, 1));
     	PUT(HDRP(NEXT_BLKP(ptr)), PACK(prevSize - size, 1));
     	mm_free(NEXT_BLKP(ptr));
     	return ptr;
     }

     ptr2 = mm_malloc(size);
     if(!ptr2) return 0;
     if (size < prevSize) prevSize = size;
     memcpy(ptr2, ptr, prevSize);
     mm_free(ptr);
     return ptr2;
}

/*******************
 * HELPER FUNCTIONS
 ******************/

//Extends the heap with a specified amount
static void* extend_heap(size_t words)
{
	char* bp;
	size_t size;

	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

	if (size < OVERHEAD) size = OVERHEAD;

	if((long)(bp = memsbrk(size)) == -1) return NULL;

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));

	return coalesce(bp);
}


static void* coalesce(void *bp)
{
	size_t previous_alloc = GET_ALLOC(FTRP(PREVBLKP(bp))) || PREV_BLKP(bp) == bp;
	size_t next__alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if(previous_alloc && !next__alloc)
	{
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		remove_block(NEXT_BLKP(bp));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
	else if(!previous_alloc && next__alloc)
	{
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		bp = PREV_BLKP(bp);
		remove_block(bp);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
	else if(!previous_alloc && !next__alloc)
	{
	        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));              
		remove_block(PREV_BLKP(bp));                                                        
		remove_block(NEXT_BLKP(bp));                                                        
		bp = PREV_BLKP(bp);                                                                 
		PUT(HDRP(bp), PACK(size, 0));                                                       
		PUT(FTRP(bp), PACK(size, 0)); 

	}

	insert_at_front(bp);
	return bp;
}

static void place(void* bp, size_t size)
{
	size_t totalsize = GET_SIZE(HDRP(bp));

	if((totalsize - size) >= OVERHEAD)
	{
		PUT(HDRP(bp), PACK(size, 1));
		PUT(FTRP(bp), PACK(size, 1));
		remove_block(bp);
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(totalsize - size, 0));
		PUT(FTRP(bp), PACK(totalsize - size, 0));
		coalesce(bp);
	}
	else
	{
		PUT(HDRP(bp), PACK(totalsize, 1));
		PUT(FTRP(bp), PACK(totalsize, 1));
		remove_block(bp);
	}
}

static void* find_fit(size_t size)
{
	void* bp;
	
	for(bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp))
	{
		if(size <= GET_SIZE(HDRP(bp))) return bp;
	}

	return NULL;
}



static void  insert_at_front(void *bp){
	        NEXT_FREEP(bp) = free_listp;
		        PREV_FREEP(free_listp) = bp;
			        PREV_FREEP(bp) = NULL;
				        free_listp = bp;
}

static void  remove_block(void *bp){
	        if(PREV_FREEP(bp)){
			                if(PREV_FREEP(bp)){
						                        NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);}
					        }
		        else{
				                free_listp = NEXTFREEP(bp);
						        }
			        PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
}

static int   check_block(void *bp){

	if(NEXT_FREEP(bp) < mem_heap_lo() || NEXT_FREEP(bp) > mem_heap_hi()){
		        printf("Fatal: Next free pointer %p is out of bounds\n", NEXT_FREEP(bp));
			        return -1;
				    }

	    if(PREV_FREEP(bp) < mem_heap_lo() || PREV_FREEP(bp) > mem_heap_hi()){
		                        printf("Fatal: Previous free pointer %p is oiut of bounds", PREV_FREEP(bp));
					                            return -1;
								                                    }

	            if((size_t)bp % 8){
			                            printf("Fatal: %p is not aligned", bp);
						                                    return -1;
										                                        }

		                if(GET(HDRP(bp)) != GET(FTRP(bp))){
					                            printf("Fatal: Header and footer mismatch");                                  
								                                        return -1;
													                                        }
				                return 0;
}
