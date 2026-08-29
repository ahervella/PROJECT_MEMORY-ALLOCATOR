Memory Allocator

This is a project in which I explored making two memory allocators. Both of these stemed from the 2008 IBM developer tutorial page for memory managers by ___ and ___, along with my own unique tailoring.

In the following I will discuss:
- The implementation of each allocator, including benefits, drawbacks, and future improvements.
- Oversights found in the IBM developer page
- My changes that differed from the IBM page
- The timed results of my test bench against the default use of new and delete

The Allocators

As with most custom allocators found online, these two allocators take advantage of overriding the new and delete operators in C++, and both rely on the concept of requesting large cached pools of memory from the OS's virtual memory that are held onto and recycled by the program so as to avoid the need to allocate and free from the OS as frequently.

Single Class Basic Pool Allocator:

This allocator is essentially a linked list chain of block with a fixed size with a predefined chain (memory pool) size. An allocator instnace is designed to be for only one class type, with each block sized to hold a single an instance of this class type.

To keep track of the linked list, all blocks in the pool that are free for use are recasted to be a FreeStore structure, with the allocator instance only keeping track of the head. When a pool is created or expanded, the allocator requests block sizes of memory in the form of FreeStore instances, where the first instance is the head, and following instances are set to be the next instance of the previous one.

When a block is allocated, the allocator returns recasts the head block, but first sets the head's next block to be the new head block. If there is no new head block, the pool is expanded again.

When a block is freed, the allocator recasts the instance back to a FreeStore instance, assigns it as the new head, and the previous head as it's next block. This means that the linked list may not necessarily be in the same order as it's pointer addresses from head to tail.

This method is clever in taking advantage of the unused pool blocks to house the needed meta data (apart form the allocator's head).

- no expansion for arrays
- one class type per allocator instance
- block needs to be at least the size of the FreeStore stuct, downside if class type specific to is smaller than that

Bit Map Pool Allocator

This allocator also uses preallocated pools of blocks with a fixed size, but instead of a linked list, an array of ints is used to act as a bitmap to denote whether a block is free or not.

Upon expanding the pool, we allocate a "chunk" from the OS, and divide that chunk into equal block sizes for our designated class size. Each chunk allocated has it's metadata stored in a bitmap entry class. Each entry holds a bitmap in the form of an array of ints is created and set all bits to 1 to indicate free, and a few helper functions.

When a block is allocated, the first free block's pointer is returned to be casted to as an instance of the assigned class type, and the bitmap bit is flipped to mark as occupied.

When a block is freed, the corresponding bit is simply flipped back to indicate available, and all data stored at that pointer can be ignored and later replaced once reused from there on out.

Additionally, we have a chunks dedicated just to allocating arrays with our program for a designated class. The allocator has a dedicated map to indicate the corresponding array pointer head in a of the array chunks, to the array's meta data in the form of a ArrayMemoryInfo class, including the length and chunk head offset. The implementation uses this info to allocate multiple contiguous blocks at once, since this bitmap method can gaurantee that the order of the blocks reflects the order of the OS memory allocated. 

Currently, freed arrays are kept track of on the allocator to be recycled for use by future array allocations that fit in any previously freed array space. All of this means there are improvments and features needed for this part of the allocator to be more practical, namely:

- Omit the use of wasting unused space when a future allocating array occupies the space of a previously freed larger array, which can be achieved by (next bullet)
- Allow freed arrays to be merged with other neighboring free arrays so as to not leave unnecessary holes in the array designated chunks (or come up with a new solution that can communicate the size of gaps in chunks)
- Allow for arrays to span across multiple chunks if they are allocated towards the end of a chunk (currently a new chunk would be allocated to accomodate this), or if they are simply larger than an entire chunk.

IBM Tutorial:
https://developer.ibm.com/tutorials/au-memorymanager/

Stack Overflow Discussion too:
https://stackoverflow.com/questions/4642671/c-memory-allocators?utm_source=chatgpt.com
