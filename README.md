# Illustration of Memory Paging in Linux

The idea is we will create a victim process, then use some linux tools to
monitor its memory access pattern and influence.

The two main tools are `/proc/{pid}/pagemap` file and the `process_madvise` syscall.
The `pagemap` and `maps` are metadata about running processes, and we can use them
to see which pages have been accessed by the victim process.

The `process_madvise` syscall allows one process to influence the memory handling of another. 
https://man7.org/linux/man-pages/man2/process_madvise.2.html


## Parsing the pagemaps

The pagemaps.py is a very simple python script used to illustrate how we can
inspect the pagetables of a running process.

First it parses the listings from `/proc/{pid}/maps` which is human readable,
and has one line for each range. It indicates which are backed by files, which
are for the stack and the heap, and which are anonymous or occupy static
areas of the program.

Next it traverses the more detailed `/proc/{pid}/pagemap` structure to read more of the page table entries. This indicates whether a given page is
"present", meaning backed by physical memory, or "non-present" meaning swapped out

For a reference, look here:
https://www.kernel.org/doc/Documentation/vm/pagemap.txt

## Unpaging the pagemaps

The swapout tool iterates through the pagemaps, and uses `process_madvise` syscall. This is present in Linux 5.10 and on.

This code comes from this Stack Overflow answer: https://serverfault.com/questions/938431/is-there-a-way-to-force-a-single-process-to-swap-most-all-of-its-memory-to-disk

## Run a demo in Tmux

The makefile builds both the victim program (cprograms/gpt1) and the swapout tool.

Here's an illustration:
```bash
   sh tmuxdemo.sh
```

This does the following:
- Runs the victim program, storing the PID in `mypid`. Type "Draw 12" to access pages in a pattern
- Displays the histogram of which pages are present (on repeat)
- Displays a text render of the pagemap ranges (on repeat)
- (With sudo) Swap out all the pages of the victim using madvise