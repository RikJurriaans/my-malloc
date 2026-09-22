Heap-dump checklist

- [x] Walk the full linked list from base via next, not just the first block
- [x] Print a one-line structural summary per block: index, address, free, size, next, prev
- [x] Only print the raw byte dump for blocks where free == 0 (freed blocks hold stale/garbage bytes)
- [x] Number the blocks (Block 0, Block 1, ...) so you can refer to them easily when describing test results
- [x] Visually distinguish free vs. allocated blocks (e.g. a marker/label) so it's obvious at a glance
- [ ] Gate the byte dump behind a verbosity flag/param, so a multi-block heap doesn't flood the terminal by default
     - Since this is a toy problem I'm just leaving this.
- [ ] Print something explicit when the heap is empty (base == NULL) — you already have this, just carry it into the new loop
     - Do you mean when all blocks in the heap are empty?

- [ ] Try to debug why free is set to 4
