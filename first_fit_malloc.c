#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * General notes:
 * - This has so far been a phenomenal exercise
 * - I need to somehow test this code
 * - Theres a whole series of streams with low-level that does exactly this, it's probably worth looking at these too to learn more.
 * - A potential next step after this tutorial is to make a garbage collector
 * - What else can I do in virtual memory to really understand what I'm doing
 * - How can I further improve this malloc implementation becuase I'm sure it's wrong...
 *
 * This is also part of the journey. I'll start with this implementation, then by myself I do more research, maybe I'll rebuild it a couple of times... Or I get bored with it and move on to the next thing, some parts will be sticky others not so much.
 * I'll probably try to work with this, print the heap, test it. Maybe write a blog post about it. Then I need to build on it by picking another exercise that teaches me a little bit more about the heap and memory.
 */

// This is pretty important, but I don't fully understand it and I wouldn't have been able to derive this...
// Meaning I still need to learn a lot about virtual memory
#define align4(x) (((((x) - 1) >> 2) << 2) + 4)

typedef struct s_block *t_block;

// The base address from where we start adding addresses to the heap.
void *base = NULL;

struct s_block {
  size_t          size;
  struct s_block *next;
  struct s_block *prev;
  int             free;
  void           *ptr;
  char            data[1];
};

#define BLOCK_SIZE 20

// Some helper function prototypes because their implementation is not super important
// they're very simple anyway
t_block find_block(t_block *last, size_t size);
t_block extend_heap(t_block last, size_t s);
t_block fusion(t_block b);
t_block get_block(void *p);
void copy_block(t_block src, t_block dst);
void split_block(t_block b, size_t s);
int is_valid_addr(void *p);

// malloc reserves a chunk of memory as large as "size"
// if it can't it returns NULL
void *malloc(size_t size) {
  t_block         b, last;
  size_t          s;
  s = align4(size);

  // check the global base variable is not null
  if (base) {
    last = base;
    b = find_block(&last, s);
    if (b) {
      // can we split?
      if ((b->size - s) >= (BLOCK_SIZE + 4)) {
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

void free(void *p) {
  t_block b;
  if (is_valid_addr(p)) {
    b = get_block(p);
    b->free = 1; // set our free flag to free.
    // now we check if our previeous is free?
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

// calloc does malloc for the "size" but times the "number" variable both of which are size_t
// This is useful if you want to allocate space for an array for example
void *calloc(size_t number, size_t size) {
  size_t        *new;
  size_t         s4,i;
  new = malloc(number * size);
  if (new) {
    // What does this one exactly mean?
    s4 = align4(number * size) << 2;
    for (i = 0; i < s4; i++) {
      new[i] = 0;
    }
  }
  return new;
}

// realloc
void *realloc(void *p, size_t size) {
  size_t          s;
  t_block         b,new;
  void           *newp;
  // This is funny, this is expected behaviour of realloc, when your ptr is NULL, realloc is basically malloc
  if (!p) {
    return malloc(size);
  }
  if (is_valid_addr(p)) {
    s = align4(size);
    b = get_block(p);
    if (b->size >= s) {
      if (b->size - s >= (BLOCK_SIZE + 4)) {
        split_block(b, s);
      }
    } else {
      if (b->next && b->next->free &&
          (b->size + BLOCK_SIZE + b->next->size) >= s) {
        fusion(b);
        if (b->size - s >= (BLOCK_SIZE + 4)) {
          split_block(b, s);
        }
      } else {
        newp = malloc(s);
        if (!newp) {
          return NULL;
        }
        new = get_block(newp);
        copy_block(b, new);
        free(p);
        return newp;
      }
    }
    return p;
  }
  return NULL;
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

// fusions interface is a bit confusing...
// what it basically does is take a block and merge is with the next one.
t_block fusion(t_block b) {
  if (b->next && b->next->free) {
    b->size += BLOCK_SIZE + b->next->size;
    b->next = b->next->next;
    if (b->next) {
      b->next->prev = b;
    }
  }
  return b;
}

t_block get_block(void *p) {
  char *tmp;
  tmp = p;
  return (p = tmp -= BLOCK_SIZE);
}

int is_valid_addr(void *p) {
  if (base) {
    if (p > base && p < sbrk(0)) {
      return p == (get_block(p))->ptr;
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
  int     sb;
  t_block b;

  b = sbrk(0);
  sb = (int)sbrk(BLOCK_SIZE + s);

  // check if we can reserve a block of memory
  if (sb < 0) {
    return NULL;
  }

  // add size to the meta
  b->size = s;
  // make b next the last one in the chain
  b->next = NULL;
  b->prev = last;
  b->ptr = b->data;
  if (last) {
    last->next = b;
  }
  b->free = 0; // not free

  return b;
}

void split_block(t_block b, size_t s) {
  t_block new;
  // oh data[1] is basically the address of the end of the block!!
  new = (t_block)(b->data + s);
  // I don't really get this math here.
  new->size = b->size - s - BLOCK_SIZE;
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

void dump_header(t_block b) {
  printf("| address: %p, free: %d, size: %zu bytes, next: %p, prev: %p\n", 
      b, b->free, b->size, b->next, b->prev);
}

void dump_block(t_block b) {
  int i, total;

  dump_header(b);
  printf("| value of bytes: \n");

  // Now printing the value on this memory address is very interesting
  // b->data holds the address of the data (returned by malloc)
  // b->data[0], 1, 2, 3 hold individual bytes of data
  // all in binary ofcourse, stored in little endian (because of my computer)
  // 
  // So we need to read them byte by byte
  // and some how add these numbers up
  // to do this we need to shift the bits into position
  //
  // An int in memory takes 4 bytes a byte is 8 bits
  // so each byte grouping needs to be shifted by 8
  // because it's little endian the order is reversed
  // (data[3] << 32) (data[2] << 16) (data[1] << 8) (data[0])
  
  total = 0;
  unsigned char *content = b->data;
  for (i = b->size-1; i >= 0; i--) {
    total |= content[i] << (8 * i);
    printf("%x, \n", content[i]);
  }

  printf("%d\n", total);
}

void dump_heap() {
  printf(  " =========================Heap=========================\n");
  if (!base) {
    printf("| Heap is empty\n");
    printf(" ======================================================\n");
    return;
  }

  // I don't know if this is the best way or not, but it is a way.
  // we walk back from the base to the head of the free list.
  t_block cur;
  cur = base;

  // This will be a while loop after
  while (cur != NULL) {
    // wouldn't it be cool if I can find out the type of data
    dump_block(cur);
    if (cur->next) {
      printf(  " ------------------------------------------------------\n");
    }
    cur = cur->next;
  }

  printf(" ======================================================\n");
}

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

  // First I need a utility function that can print the current heap layout
  int *a;

  a = malloc(sizeof(int));
  if (a == NULL) {
    printf("ERROR: malloc failed.\n");
  }

  *a = 12332112; 

  int *b;

  b = malloc(sizeof(int));
  if (b == NULL) {
    printf("ERROR: malloc failed.\n");
  }

  *b = -65433; 

  //char *c;

  //c = malloc(sizeof(char) * 4);
  //if (c == NULL) {
  //  printf("ERROR: malloc failed.\n");
  //}
  //
  //strcpy(&c, "Rik");

  dump_heap();
  printf("\n\n\n");


  free(a);
  free(b);
  //free(c);

  return 0;
}
