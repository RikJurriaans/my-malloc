Here's a practical toolkit for hunting a segfault, from lowest to highest effort:

1. Compile with debug info and full warnings
gcc -g -Wall -Wextra -o malloc_test first_fit_malloc.c
-g keeps symbol/line info so any debugger can map crashes back to your source. Don't skip -Wall -Wextra — it'll sometimes flag the exact kind of pointer/type mistake that leads to a segfault before you even run it.

2. GDB — the direct way to catch it in the act
gdb ./malloc_test
(gdb) run
When it crashes, GDB drops you right back at the prompt. Then:
- bt — backtrace: shows the call stack at the moment of the crash (which function called which function called which)
- bt full — same, but also prints every local variable in each stack frame — extremely useful for seeing exactly what a pointer was pointing to right before it blew up
- frame N — jump to a specific frame in the backtrace to inspect it
- print b, print *b, print b->next — inspect any variable or dereference a struct pointer directly, even mid-crash
- list — show the source around the current line

3. AddressSanitizer — usually the fastest path to a root cause
gcc -g -fsanitize=address -o malloc_test first_fit_malloc.c
./malloc_test
ASan instruments every memory access and will print a detailed report the instant something goes wrong (out-of-bounds read/write, use-after-free, invalid free) — including the exact file/line of the bad access and the file/line of the allocation it belongs to. This tends to be far more precise than a bare segfault + backtrace, especially for off-by-one or stale-pointer bugs. Since you've renamed your allocator functions, ASan's own instrumentation won't collide with your malloc/free symbols anymore, so it should work cleanly now.

4. Core dumps — for post-mortem debugging without needing to catch it live
ulimit -c unlimited
./malloc_test
gdb ./malloc_test core
Useful if the crash happens somewhere inconvenient to reproduce interactively (e.g. deep in a loop).

5. Since it's "occasional" — think about what varies between runs
Intermittent crashes on the same code usually mean the exact memory layout matters (heap addresses shift slightly run to run due to ASLR — address space layout randomization). A bug like an out-of-bounds read might silently succeed on one run and hit unmapped memory on the next, purely because of where the heap happened to land. You can make runs more repeatable to help you catch it:
setarch $(uname -m) -R ./malloc_test
This disables ASLR for that one run, so the same underlying bug should reproduce more consistently.

6. Caveat for this specific program
Valgrind is normally a go-to for memory bugs, but your program calls sbrk/brk directly to manage its own heap — Valgrind manages the heap itself under the hood, so mixing raw sbrk calls with Valgrind's instrumentation can behave unpredictably. ASan + GDB are the more reliable combo here.

Start with #3 (ASan) — for a segfault in hand-rolled pointer/heap code, it'll almost always point you straight at the offending line.
