#include <stdio.h>
#include <stdlib.h>

/**
 * Oke so the lesson here is that if you convert an address of 4 bytes
 * and then cast it into an int the address truncates.
 */
int main(void) {
  void *p = malloc(4);   // use the REAL malloc here, just to get a real address
  printf("full pointer: %p\n", p);
  int truncated = (int)(long)p;   // force the same truncation
  printf("truncated to int: %d\n", truncated);
  return 0;
}

