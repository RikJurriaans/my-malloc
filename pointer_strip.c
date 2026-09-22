#include <stdio.h>

int main(void) {
  int i;
  unsigned int patterns[] = {0x00000001, 0x7FFFFFFF, 0x80000000, 0xD1499000, 0xFFFFFFFF};
  for (i = 0; i <= 5; i++) {
    unsigned int u  = patterns[i];
    int s = (int)u;
    printf("0x%o8X as unsigned = %u, as signed %d\n", u, u, s);
  }
  return 0;
} 
