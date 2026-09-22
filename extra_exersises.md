Here are three small, standalone exercises — each isolates one piece of what you just fixed, so you can poke at it without the full malloc project in the way.

Exercise 1 — Watch the truncation happen

Write a tiny program:
#include <stdio.h>
#include <stdlib.h>

```
int main(void) {
  void *p = malloc(4);   // use the REAL malloc here, just to get a real address
  printf("full pointer: %p\n", p);
  int truncated = (int)(long)p;   // force the same truncation
  printf("truncated to int: %d\n", truncated);
  return 0;
}
```

Before running it, manually take the hex address it prints, drop everything except the last 8 hex digits, and predict whether the result will look negative (hint: check the very first bit of those last 8 digits). Then run it and see if your prediction matches. Repeat a few times (addresses change each run) until you can reliably predict positive vs. negative just by looking at the top hex digit.

Exercise 2 — Sign bit in isolation, no pointers at all

This strips away pointers entirely so you can see it's purely about bit patterns and types, nothing malloc-specific:
#include <stdio.h>

int main(void) {
  unsigned int patterns[] = {0x00000001, 0x7FFFFFFF, 0x80000000, 0xD1499000, 0xFFFFFFFF};
  for (int i = 0; i < 5; i++) {
    unsigned int u = patterns[i];
    int s = (int)u;
    printf("0x%08X as unsigned = %u, as signed = %d\n", u, u, s);
  }
  return 0;
}
Before running, for each value, look only at the top 4 bits (first hex digit) and predict sign: 0-7 → non-negative, 8-F → negative. Then verify against the actual output. This is the exact mechanism that made sb < 0 misfire — you're just seeing it without the sbrk/malloc noise around it.

Exercise 3 — Write the check the "right" way vs the "wrong" way

Simulate the actual bug pattern in miniature. Write a function that pretends to be sbrk: it returns a void* most of the time, but returns (void*)-1 to signal failure sometimes (e.g. based on an argument you pass in, so you can force both cases on demand). Then write two versions of the caller:
- One that copies the return value into an int first, then checks < 0 (the buggy pattern)
- One that checks the return value directly against (void*)-1, no int involved (the fixed pattern)

Call both versions with a normal heap-ish address (something like 0x00007f0000001000 — pick something with the sign bit set in its low 32 bits) and confirm: the buggy version misreports it as a failure, the fixed version correctly recognizes it as success. Then call both with an actual (void*)-1 and confirm both correctly detect the real failure.

Exercise 3 is the closest to what you actually fixed — it should make it click why the fix works, since you'll have built both the broken and working version side by side yourself.
