#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

/**
 * General notes:
 * - This has so far been a phenomenal exercise
 * - I need to somehow test this code
 * - Theres a whole series of streams with low-level that does exactly this, it's probably worth looking at these too to learn more.
 * - A potential next step after this tutorial is to make a garbage collector
 * - What else can I do in virtual memory to really understand what I'm doing
 * - How can I further improve this malloc implementation becuase I'm sure it's wrong...
 *
 * - I would say however that this is really not easy, actually pretty fucking difficult exercise
 *
 * This is also part of the journey. I'll start with this implementation, then by myself I do more research, maybe I'll rebuild it a couple of times... Or I get bored with it and move on to the next thing, some parts will be sticky others not so much.
 * I'll probably try to work with this, print the heap, test it. Maybe write a blog post about it. Then I need to build on it by picking another exercise that teaches me a little bit more about the heap and memory.
 *
 *
 * Definitions:
 * - block: a full memory block with header and memory section
 * - heap: the full space of virtual memory
 * - header: the section in our block that contains meta data about the memory block to make free possible
 */

// This is pretty important, but I don't fully understand it and I wouldn't have been able to derive this...
// Meaning I still need to learn a lot about virtual memory
#define align4(x) (((((x) - 1) >> 2) << 2) + 4)

typedef struct s_block *t_block;

// The base address from where we start adding addresses to the heap.
void *base = NULL;

// Structs are always aligned, they will automatically add padding
struct s_block {
  size_t          size;
  struct s_block *next;
  struct s_block *prev;
  // thats why it doesn't matter for this to be a 4 byte type
  int             free;
  void           *ptr;
  char            data[1];
};

// This is the size of the header
#define HEADER_BLOCK_SIZE 40

// Some helper function prototypes because their implementation is not super important
// they're very simple anyway
t_block find_block(t_block *last, size_t size);
t_block extend_heap(t_block last, size_t s);
t_block fusion(t_block b);
t_block get_header_block(void *p);
void copy_block(t_block src, t_block dst);
void split_block(t_block b, size_t s);
int is_valid_addr(void *p);

// malloc reserves a chunk of memory as large as "size"
// if it can't it returns NULL
void *my_malloc(size_t size) {
  t_block         b, last;
  size_t          s;
  s = align4(size);

  // check the global base variable is not null
  if (base) {
    last = base;
    b = find_block(&last, s);
    if (b) {
      // can we split?
      if ((b->size - s) >= (HEADER_BLOCK_SIZE + 4)) {
        split_block(b, s);
      }
      b->free = 0;
    } else {
      /* No fitting block, extend the heap */
      b = extend_heap(last, s);
      if (!b) {
        return NULL;
      }
    }
  } else {
    // theres no heap now, so we need to reserve the first block with size s
    // and return it
    b = extend_heap(NULL, s);
    // if this fails we return NULL
    if (!b) {
      return NULL;
    }
    // we set the newly allocated block to the global base variable
    base = b;
  }

  // return the data from our block, the meta data isn't exposed to the outside
  return b->data;
}

void my_free(void *p) {
  t_block b;
  if (is_valid_addr(p)) {
    b = get_header_block(p);
    b->free = 1; // set our free flag to free.
    if (b->prev && b->prev->free) {
      b = fusion(b->prev);
    }
    if (b->next) {
      fusion(b);
    } else {
      // we don't have a next block, so we're at the end of the heap.
      // we can now shrink the heap again.
      // if we have a previous chunk in our freelist
      if (b->prev) {
        // we set ourselves to null, losing the reference
        b->prev->next = NULL;
      } else {
        // if we do not have a prev, we're at the beginning of the heap 
        // so we reset the base pointer of our heap to NULL, starting over essentially.
        base = NULL;
      }
      brk(b); // set the breakpoint to the base address regardless. Shrinking the heap.
    }
  }
}

void copy_block(t_block src, t_block dst) {
  int          *sdata, *ddata;
  size_t        i;
  sdata = src->ptr;
  ddata = dst->ptr;
  for (i=0; i * 4 < src->size && i * 4 < dst->size; i++) {
    ddata[i] = sdata[i];
  }
}

// fusions interface i+ s a bit confusing...
// what it basically does is take a block and merge is with the next one.
t_block fusion(t_block b) {
  if (b->next && b->next->free) {
    b->size += HEADER_BLOCK_SIZE + b->next->size;
    b->next = b->next->next;
    if (b->next) {
      b->next->prev = b;
    }
  }
  return b;
}

t_block get_header_block(void *p) {
  char *tmp;
  tmp = p;
  return (p = tmp -= HEADER_BLOCK_SIZE);
}

int is_valid_addr(void *p) {
  if (base) {
    if (p > base && p < sbrk(0)) {
      return p == (get_header_block(p))->ptr;
    }
  }
  return 0;
}

t_block find_block(t_block *last, size_t size) {
  t_block b = base;
  while (b && !(b->free && b->size >= size)) {
    *last = b;
    b = b->next;
  }
  return b;
}

t_block extend_heap(t_block last, size_t s) {
  t_block b;
  b = sbrk(0);

  if (sbrk(HEADER_BLOCK_SIZE + s) == (void*)-1) {
    return NULL;
  }
  b->size = s;
  b->next = NULL;
  if (last) {
    last->next = b;
  }
  b->free = 0;
  return b;
}

void split_block(t_block b, size_t s) {
  t_block new;
  new = (t_block)(b->data + s);
  new->size = b->size - s - HEADER_BLOCK_SIZE;
  new->next = b->next;
  new->prev = b;
  new->free = 1;
  new->ptr = new->data;
  b->size = s;
  b->next = new;
  if (new->next) {
    new->next->prev = new;
  }
}

void dump_header(int block_number, t_block b) {
  printf("| block_number: %d, address: %p, free: %d, size: %zu bytes, next: %p, prev: %p\n", 
      block_number, b, b->free, b->size, b->next, b->prev);
}

void dump_block(t_block b) {
  int i;

  unsigned char *content = b->data;
  for (i = b->size-1; i >= 0; i--) {
    printf("%x, \n", content[i]);
  }
}

void dump_heap() {
  int block_number;

  printf(" =========================Heap=========================\n");
  if (!base) {
    printf("| Heap is empty\n");
    printf(" ======================================================\n");
    return;
  }

  // I don't know if this is the best way or not, but it is a way.
  // we walk back from the base to the head of the free list.
  t_block cur;
  cur = base;
  block_number = 0;

  while (cur != NULL) {
    dump_header(block_number, cur);
    if (cur->free == 0) {
      dump_block(cur);
      if (cur->next) {
        printf(" ------------------------------------------------------\n");
      }
    }
    cur = cur->next;
    block_number++;
  }

  printf(" ======================================================\n");
}

// Tests
// - we need to test that malloc works, allocates some memory that should be the exact size of what we requested, free flag needs to be 1
// - we need to test freeing that memory, the free flag of that block should be set to 1
// - we need to test mallocing 2 variables, you should be able to see both of them in memory with the correct headers
int main(void) {
  // the biggest dissapointment of this tutorial I think is the lack of
  // testing that you do, I much rather test step by step and thats how
  // I build programs.
  //
  // And I think that might be a great opportunity for a blog post of my
  // own... I can create a blog post in which I walk through this, sharing
  // what I've learned from doing a couple of malloc tutorials
  //
  // Walking the readers through the steps, testing the different functions.
  
  printf("sizeof(struct s_block) = %zu\n", sizeof(struct s_block));
  printf("offsetof(data)         = %zu\n", offsetof(struct s_block, data));

  // First I need a utility function that can print the current heap layout
  int *a;

  a = my_malloc(sizeof(int));
  if (a == NULL) {
    printf("ERROR: malloc failed.\n");
    return 1;
  }

  *a = 12332112; 

  int *b;

  b = my_malloc(sizeof(int));
  if (b == NULL) {
    printf("ERROR: malloc failed.\n");
    return 1;
  }

  *b = -65433; 

  dump_heap();
  printf("\n\n\n");

  my_free(a);
  my_free(b);

  // TODO: fix free

  dump_heap();
  printf("\n\n\n");

  return 0;
}
