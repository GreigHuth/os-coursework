# os-coursework
Operating Systems Coursework

The coursework involved implementing key components of the InfOS operating system (https://github.com/tspink/infos). 
I was required to implement the following:

1 - A round-robin process scheduler.

2 - The physical memory page allocator, as per the buddy system for memory allocation. https://en.wikipedia.org/wiki/Buddy_memory_allocation

3 - Device driver for a real time CMOS clock.  

Each file was a given template with blank functions we were required to implement, i implemented the following functions:

pick_next_entity() -  this function choses which CPU task to run next, based on "Round Robin" scheduling

split_block() - split a block of memory in half

init() -  Initialises the allocation algorithm.

reserve_pages() - reserve specific pages so they cant be allocated 

alloc_pages() - allocates contiguous pages of memory in a given order

merge_block() - merge two blocks of memory and create a new block

free_pages() - free contiguous pages of memory

read_timepoint() - asks the RTC for the current date and time

