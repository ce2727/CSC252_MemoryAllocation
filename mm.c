/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never merge_blocksd or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

#include < stdio.h > #include < stdlib.h > #include < assert.h > #include < unistd.h > #include < string.h >

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
#
define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
# define ALIGN(size)(((size) + (ALIGNMENT - 1)) & ~0x7)

# define SIZE_T_SIZE(ALIGN(sizeof(size_t)))

# define WORD 4
# define DWORD 8
# define HEAPINIT 16# define MIN_BLOCK 24
# define GET(p)( * (size_t * )(p))# define WRITE(p, value)( * (size_t * )(p) = (value))
# define MAX(x, y)((x) > (y) ? (x) : (y))# define ADD_SIZE(size, alloc)((size) | (alloc))

# define GET_HEADER(ptr)((void * )(ptr) - WORD)
# define GET_FOOTER(ptr)((void * )(ptr) + GET_SIZE(GET_HEADER(ptr)) - DWORD)
# define GET_SIZE(p)(GET(p) & ~0x7)
# define GET_ALLOC(p)(GET(p) & 0x1)

# define NEXT_BLOCK(ptr)((void * )(ptr) + GET_SIZE(GET_HEADER(ptr)))
# define PREVIOUS_BLOCK(ptr)((void * )(ptr) - GET_SIZE(GET_HEADER(ptr) - WORD))
# define NEXT_OPEN(ptr)( * (void * * )(ptr + DWORD))# define PREV_OPEN(ptr)( * (void * * )(ptr))

static char * block_ptr = 0;
static char * free_block_ptr = 0;

//Helper function headers
static void * heap_expand(size_t words);
static void * block_fit(size_t size);
static void put_block(void * ptr, size_t size);
static void delete_block(void * ptr);
static int is_consistent(void * ptr);
static void * merge_blocks(void * ptr);
static void prepend(void * ptr);

/**************************
 * MAIN FUNCTIONS
 **************************/

/* 
 * mm_init - initialize the malloc package.
 */
 int mm_init(void)
 {
     if ((block_ptr = mem_sbrk(MIN_BLOCK * 2)) == NULL) return -1;//Return on heap failure
     
     WRITE(block_ptr, 0);
     WRITE(block_ptr + WORD, ADD_SIZE(MIN_BLOCK, 1));
     WRITE(block_ptr + DWORD, 0);
     WRITE(block_ptr + DWORD + WORD, 0);
     WRITE(block_ptr + MIN_BLOCK, ADD_SIZE(MIN_BLOCK, 1));
     WRITE(block_ptr + WORD + MIN_BLOCK, ADD_SIZE(0,1));
     
     free_block_ptr = DWORD + block_ptr;
 
     if(heap_expand(HEAPINIT / WORD) == 0) return -1;//Make sure it didn't break
 
     return 0;
 }
 
 /* 
  * mm_malloc - Allocate a block by incrementing the brk pointer.
  *     Always allocate a block whose size is a multiple of the alignment.
  */
 void *mm_malloc(size_t size)
 {
     	size_t correctedWord;
 	size_t maxWord;
 	char* ptr;
 
 	if(size <= 0) return NULL;
 
 	correctedWord = MAX(ALIGN(size) + DWORD, MIN_BLOCK);
 
 	if((ptr = ((void*) block_fit(correctedWord))))
 	{
 		put_block(ptr, correctedWord);
 		return ptr;
 	}
 
 	maxWord = MAX(correctedWord, HEAPINIT);
 
 	if((ptr =(void*) heap_expand(maxWord / WORD)) == NULL) return NULL;
 
 	put_block(ptr, correctedWord);
 	return ptr;
 }
 
 /*
  * mm_free - Freeing a block does nothing.
  */
 void mm_free(void *ptr)
 {
 	if(!ptr) return;
 	size_t size = GET_SIZE(GET_HEADER(ptr));
 	WRITE(GET_HEADER(ptr), ADD_SIZE(size, 0));//Simply sets lengths of headers/footers to 0
 	WRITE(GET_FOOTER(ptr), ADD_SIZE(size, 0));
 	merge_blocks(ptr);//Absorb any surrounding free blocks, if any
 }
 
 /*
  * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
  */
 void *mm_realloc(void *ptr, size_t size)
 {
     size_t prevSize = 0;
     void* ptr2;
     size_t newWord = MAX(ALIGN(size) + DWORD, MIN_BLOCK);
     
     if(size <= 0)
      { 
 	     mm_free(ptr); 
 	     return 0;
      }
 
      if(ptr == NULL)
      {
 	     return mm_malloc(size);
      }
 	
      prevSize = GET_SIZE(GET_HEADER(ptr));
 
      if(prevSize == newWord) return ptr;
      if(newWord <= prevSize)
      {
 	size = newWord;
      	if(prevSize - size <= MIN_BLOCK) return ptr;
 
     	WRITE(GET_HEADER(ptr), ADD_SIZE(size, 1));
      	WRITE(GET_FOOTER(ptr), ADD_SIZE(size, 1));
      	WRITE(GET_HEADER(NEXT_BLOCK(ptr)), ADD_SIZE(prevSize - size, 1));
      	mm_free(NEXT_BLOCK(ptr));
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
static void * heap_expand(size_t words) {
  char * ptr;
  size_t size;
  size = (words % 2) ? (words + 1) * WORD : words * WORD;

  if (size < MIN_BLOCK)
    size = MIN_BLOCK;

  if ((long)(ptr = (void * ) mem_sbrk(size)) == -1)
    return NULL;

  WRITE(GET_HEADER(ptr), ADD_SIZE(size, 0));
  WRITE(GET_FOOTER(ptr), ADD_SIZE(size, 0));
  WRITE(GET_HEADER(NEXT_BLOCK(ptr)), ADD_SIZE(0, 1));

  return merge_blocks(ptr);
}

static void * merge_blocks(void * ptr) {
  size_t previous_alloc = GET_ALLOC(GET_FOOTER(PREVIOUS_BLOCK(ptr))) || PREVIOUS_BLOCK(ptr) == ptr;
  size_t next__alloc = GET_ALLOC(GET_HEADER(NEXT_BLOCK(ptr)));
  size_t size = GET_SIZE(GET_HEADER(ptr));

  if (previous_alloc && !next__alloc) {
    size += GET_SIZE(GET_HEADER(NEXT_BLOCK(ptr)));
    delete_block(NEXT_BLOCK(ptr));
    WRITE(GET_HEADER(ptr), ADD_SIZE(size, 0));
    WRITE(GET_FOOTER(ptr), ADD_SIZE(size, 0));
  } else if (!previous_alloc && next__alloc) {
    size += GET_SIZE(GET_HEADER(PREVIOUS_BLOCK(ptr)));
    ptr = PREVIOUS_BLOCK(ptr);
    delete_block(ptr);
    WRITE(GET_HEADER(ptr), ADD_SIZE(size, 0));
    WRITE(GET_FOOTER(ptr), ADD_SIZE(size, 0));
  } else if (!previous_alloc && !next__alloc) {
    size += GET_SIZE(GET_HEADER(PREVIOUS_BLOCK(ptr))) + GET_SIZE(GET_HEADER(NEXT_BLOCK(ptr)));
    delete_block(PREVIOUS_BLOCK(ptr));
    delete_block(NEXT_BLOCK(ptr));
    ptr = PREVIOUS_BLOCK(ptr);
    WRITE(GET_HEADER(ptr), ADD_SIZE(size, 0));
    WRITE(GET_FOOTER(ptr), ADD_SIZE(size, 0));

  }

  prepend(ptr);
  return ptr;
}

static void put_block(void * ptr, size_t size) {
  size_t totalsize = GET_SIZE(GET_HEADER(ptr));

  if ((totalsize - size) >= MIN_BLOCK) {
    WRITE(GET_HEADER(ptr), ADD_SIZE(size, 1));
    WRITE(GET_FOOTER(ptr), ADD_SIZE(size, 1));
    delete_block(ptr);
    ptr = NEXT_BLOCK(ptr);
    WRITE(GET_HEADER(ptr), ADD_SIZE(totalsize - size, 0));
    WRITE(GET_FOOTER(ptr), ADD_SIZE(totalsize - size, 0));
    merge_blocks(ptr);
  } else {
    WRITE(GET_HEADER(ptr), ADD_SIZE(totalsize, 1));
    WRITE(GET_FOOTER(ptr), ADD_SIZE(totalsize, 1));
    delete_block(ptr);
  }
}

static void * block_fit(size_t size) {
  void * ptr;
  for (ptr = free_block_ptr; GET_ALLOC(GET_HEADER(ptr)) == 0; ptr = NEXT_OPEN(ptr)) {
    if (size <= GET_SIZE(GET_HEADER(ptr)))
      return ptr;
  }

  return NULL;
}

static void prepend(void * ptr) {
  NEXT_OPEN(ptr) = free_block_ptr;
  PREV_OPEN(free_block_ptr) = ptr;
  PREV_OPEN(ptr) = NULL;
  free_block_ptr = ptr;
}

static void delete_block(void * ptr) {
  if (PREV_OPEN(ptr)) {
    if (PREV_OPEN(ptr)) {
      NEXT_OPEN(PREV_OPEN(ptr)) = NEXT_OPEN(ptr);
    }
  } else {
    free_block_ptr = NEXT_OPEN(ptr);
  }
  PREV_OPEN(NEXT_OPEN(ptr)) = PREV_OPEN(ptr);
}

int mm_check(void) {
  void * ptr = block_ptr;
  printf("Heap (%p): \n", block_ptr);

  if ((GET_SIZE(GET_HEADER(block_ptr)) != MIN_BLOCK) || !GET_ALLOC(GET_HEADER(block_ptr))) {
    printf("error in header\n");
    return -1;
  }
  if (is_consistent(block_ptr) == -1) {
    return -1;
  }
  for (ptr = free_block_ptr; GET_ALLOC(GET_HEADER(ptr)) == 0; ptr = NEXT_OPEN(ptr)) {
    if (is_consistent(ptr) == -1) {
      return -1;
    }
  }

  return 0;
}

static int is_consistent(void * ptr) {

  if (NEXT_OPEN(ptr) < mem_heap_lo() || NEXT_OPEN(ptr) > mem_heap_hi()) {
    printf("Next available pointer %p is out of bounds\n", NEXT_OPEN(ptr));
    return -1;
  }
  if (PREV_OPEN(ptr) < mem_heap_lo() || PREV_OPEN(ptr) > mem_heap_hi()) {
    printf("prev available pointer %p out of bounds", PREV_OPEN(ptr));
    return -1;
  }
  if ((size_t) ptr % 8) {
    printf("%p not aligned", ptr);
    return -1;
  }
  if (GET(GET_HEADER(ptr)) != GET(GET_FOOTER(ptr))) {
    printf("header/footer don't match");
    return -1;
  }
  return 0;
}
