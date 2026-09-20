/* A horrible dummy malloc */
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>

void *malloc(size_t size) {
  void *p;

  // sbrk returns the break point in memory, the point at which theres no allocated memory in the heap

  // this line 0 returns 0 bytes from that breakpoint, so this returns the break point
  p = sbrk(0);
  // since sbrk returns the point from the breakpoint, if we add the size as the argument
  // we get the current breakpoint, until the size we want. So this bit checks if the space
  // we want to allocate is actually available
  //
  // The syntax of (void*) bs still doesn't make much sense to me though.
  if (sbrk(size) == (void*)-1) {
    return NULL;
  }

  return p;
}

int main(void) {
  int *p;

  p = malloc(sizeof(int));
  if (p == NULL) {
    printf("ERROR: my malloc failed.\n");
    return 1;
  }

  *p = 10;

  printf("%d\n", *p);

  return 0;
}

