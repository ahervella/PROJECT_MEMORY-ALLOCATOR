# Custom C++ Memory Allocators

This project is a custom C++ memory allocator project implementing two pooled allocation strategies, generalized to arbitrary types, made thread-safe, benchmarked against the default `new`/`delete`, and covered by a full GoogleTest suite with AddressSanitizer.

1. **Single-Class Pool Allocator** — an intrusive free-list-based allocator designed for fast allocation and deallocation of objects of a single type.
2. **Bitmap Pool Allocator** — a bitmap-based allocator that tracks block occupancy and additionally supports contiguous array allocations.

The project began with the 2008 IBM Developer tutorial [*Memory management: Building a memory manager*](https://developer.ibm.com/tutorials/au-memorymanager/) by Arpan Sen and Rahul Kardam as a starting point. I then substantially extended the design and implementation to explore allocation strategies, object lifetime management, array allocation, thread safety, automated testing, memory-safety validation, and performance benchmarking.

---

# Contents

- [Project Goals](#project-goals)
- [Allocator Designs](#allocator-designs)
  - [Single-Class Fixed-Size Pool Allocator](#single-class-fixed-size-pool-allocator)
- [Bitmap Pool Allocator](#bitmap-pool-allocator)
- [Array Allocation](#array-allocation)
- [Thread-Safe Variants](#thread-safe-variants)
- [Beyond the Tutorial](#beyond-the-tutorial)
  - [C++ Object Lifetime and Custom Operators](#c-object-lifetime-and-custom-operators)
  - [Extending it into something usable](#extending-it-into-something-usable)
- [My Extensions and Adaptations](#my-extensions-and-adaptations)
- [Results](#results)
- [C++ Concepts Explored](#c-concepts-explored)
  - [Memory and Object Lifetime](#memory-and-object-lifetime)
  - [Language Semantics and Type System](#language-semantics-and-type-system)
  - [Compilation and Build System](#compilation-and-build-system)
  - [Testing and Performance Tooling](#testing-and-performance-tooling)
  - [Data Structures and Low-Level Behavior](#data-structures-and-low-level-behavior)
- [Future Improvements](#future-improvements)
  - [Memory Management](#memory-management)
  - [Array Allocation](#array-allocation-1)
  - [Allocation Performance](#allocation-performance)
  - [Concurrency](#concurrency)

---

# Project Goals

The primary goals of the project were to:

- Understand how custom memory allocators manage raw memory and object lifetime.
- Implement multiple allocation strategies and compare their tradeoffs.
- Integrate custom allocators with C++ `new`, `delete`, `new[]`, and `delete[]`.
- Explore contiguous array allocation and memory fragmentation.
- Implement thread-safe and non-thread-safe variants.
- Build automated tests for allocator correctness and concurrency.
- Benchmark the allocators against the standard C++ allocator.
- Use tools such as AddressSanitizer to identify memory-management errors.

---

# Allocator Designs

## Single-Class Fixed-Size Pool Allocator

The basic pool allocator is designed for a single C++ class type. Each allocator instance manages blocks large enough to hold one instance of that type, with each block sized to hold a single an instance of this class type. When initializing or more memory is needed than is available, a large chunk or predifined size that divedes equally into block sizes is allocated.

This is done via a linked-list-of-fixed-size-blocks model. Each `BasicPoolAllocator<T>` instance manages blocks sized for exactly one type `T`.

Free blocks are recast in place as a `FreeStore` node, and the allocator only tracks the head of this list — meaning the pool's own currently-unused memory doubles as the free-list's metadata storage, with no separate bookkeeping structure needed.

This results in very little per-block bookkeeping:

```text
Free block
+-------------------+
| next free block   |
+-------------------+

Free list:

[Block] -> [Block] -> [Block] -> nullptr
   ^
   |
 head
```

### Allocation

1. Retrieve the head block as the pointer to return.
2. Reassign the head to the next free block.
3. Return the retrieved block as storage for the requested object.

If the pool has no remaining blocks, an additional predeinged large chunk (ie. chunk size = 32 blocks) is allocated and added to the free list, which allows for the allocator to remain efficient.

### Deallocation

1. Convert the object's storage to a `FreeStore` data type.
2. Set its `next` pointer to the current free-list head.
3. Make this freed block the new head.

Because freed blocks are always inserted at the head, the order of the free list does not necessarily correspond to the blocks' addresses, ie. they are not representations of adjacent, contigous memory blocks.

### Advantages

- Very low allocation/deallocation overhead.
- Minimal metadata.
- Free-list operations are effectively constant-time.
- Well suited to workloads that frequently create and destroy objects of the same size (ie. bullets in games, quick frequent vfx, etc.).

### Limitations

- No array support — arrays fall back to the OS allocator.
- One allocator instance per type (by design — see [Beyond the Tutorial](#beyond-the-tutorial)).
- Each block must be at least `sizeof(FreeStore)`, which wastes space for types smaller than a pointer.
- The pool only grows and never shrinks during the lifetime of the allocator.

### Potential Use Cases

This design is particularly suited to workloads involving large numbers of short-lived, same-sized objects, such as:

- Game projectiles or other frequently spawned entities.
- Particle/VFX objects.
- Temporary gameplay objects.
- Frequently created network or message objects.
- Other single-threaded fixed-size workloads.

---

# Bitmap Pool Allocator

The bitmap allocator uses a different strategy for tracking free blocks.

Instead of maintaining a linked list, each memory chunk is divided into equal-sized blocks and associated with a bitmap. Each bit represents whether a corresponding block is available.

```text
Memory Chunk

+--------+--------+--------+--------+-----+
| Block0 | Block1 | Block2 | Block3 | ... |
+--------+--------+--------+--------+-----+

Chunk Bitmap

  1        0        1        1       ...
 free   occupied   free     free
```

Each chunk maintains metadata describing its bitmap and block state. The allocator additionally tracks chunks containing available blocks to avoid unnecessarily searching completely occupied chunks.

### Allocation

When allocating an object:

1. Locate a chunk with available blocks.
2. Find an available bit in its bitmap.
3. Calculate the corresponding block address.
4. Mark the block as occupied.
5. Return the block as storage for the requested object.

### Deallocation

When an object is freed:

1. Determine its corresponding chunk and block.
2. Mark the block as available in the bitmap.
3. The block's previous contents are discarded and may be overwritten when reused.

### Advantages

- Explicit occupancy tracking.
- A chunk can efficiently track many blocks with relatively compact metadata.
- Blocks within a chunk have predictable addresses.
- The contiguous layout of blocks within each chunk makes contiguous array allocations possible.
- Deallocation does not require clean or recasting block (until allocated again)

### Tradeoffs

- Compared with the free-list allocator, the bitmap implementation introduces additional bookkeeping and free-block discovery work.
- Requires separate metadata for tracking allocations that span multiple blocks.
- The pool (chunk count) only grows and never shrinks during the lifetime of the allocator.

---

# Array Allocation

Array support is handled separately: dedicated chunks are reserved just for array allocations, tracked via a map from array head pointer to an `ArrayMemoryInfo` (length + chunk offset). Because the bitmap guarantees block order matches OS memory order within a chunk, arrays can be allocated as a single run of contiguous blocks. Freed arrays are tracked for reuse by future array allocations that fit in the freed space.

### Current Limitations

The current array implementation intentionally leaves several areas for future improvement:

- A smaller array reusing a larger freed region can leave unused space.
- Adjacent freed array regions are not currently coalesced into one.
- Arrays cannot span multiple chunks.
- An array larger than an individual chunk requires a new allocation strategy.

Potential improvements include maintaining the size of free regions, merging adjacent free regions, and allowing large arrays to span multiple chunks.

---

# Thread-Safe Variants

Both allocator strategies have thread-safe variants:

- `BasicPoolAllocator` -> `BasicPoolTSAllocator`
- `BitMapAllocator` -> `BitMapTSAllocator`

The thread-safe implementations protect shared allocator state using mutex synchronization. This provides a direct comparison between the allocation strategies themselves and the additional cost introduced by synchronization.

---

# Beyond the Tutorial

The IBM tutorial implements one non-generic, single-threaded allocator per strategy, hardcoded to one class (`Complex`), with no discussion of `delete`/`delete[]` overload semantics.

## C++ Object Lifetime and Custom Operators

This distinction between **memory allocation** and **object lifetime** was an important part of the project, and one of the tutorial's biggest gaps. To integrate the allocators with normal C++ object usage, the project provides custom allocation operators:

- `new`
- `new[]`
- `delete`
- `delete[]`

A custom `operator delete` with an extra allocator parameter is *not* automatically invoked by `delete` the way the built-in one is, and unlike the built-in operator, it does not call the destructor for you — both have to be done manually inside the overload. This is easy to get wrong silently: `operator delete[]` additionally can't infer how many elements it's destructing from the pointer alone, since there's no portable way to recover an array's length from a raw pointer. My overload takes an explicit length parameter (`operator delete<T>[](p, allocator, length)`) rather than relying on compiler-specific ABI array-cookie tricks, so it destructs exactly the right number of elements regardless of implementation details the standard doesn't guarantee.

## Extending it into something usable

Other features were impleneted to make it a much more practicale use case that I decided to incorperate:

- **Generic types via templates.** Both allocators are `template<class T>` classes (`BasicPoolAllocator<T>`, `BitMapAllocator<T>`) instead of being hardcoded to one type — every size comparison and cast in the tutorial's version is baked in for `Complex` specifically. Templating means one allocator implementation now serves any type, the same way `std::vector<T>` does, without touching the underlying allocation logic.
- **Thread safety.** `BasicPoolTSAllocator<T>` and `BitMapTSAllocator<T>` wrap the base allocators with mutex locking around allocate/free, so both a fast single-threaded and a safe multi-threaded variant of each strategy are available. The tutorial doesn't address concurrency at all.
- **A common interface.** All four allocators implement `IMemoryAllocator`, a small type-erased interface (`allocate`/`free`/`allocateArray`/`freeArray`), so calling code can work against any allocator polymorphically.
- **A portable, single-header integration (`allocators/customMemoryAllocator.h`).** Global placement `operator new`/`operator new[]` overloads taking an `IMemoryAllocator&`, so usage looks like ordinary C++: `new (allocator) T(args...)`.
- **A custom exception type (`BadAllocWithMsg`)** thrown on allocation failure, so failures are reported explicitly and distinguishably rather than silently returning null or aborting.
- **A full test suite** (`AllocatorTests.cpp`, GoogleTest) — typed tests that run the same allocate/free/array/oversized-request/reuse checks across all four allocator types via `TYPED_TEST_SUITE`, plus a dedicated multi-threaded stress test hammering the two thread-safe allocators concurrently and checking for address collisions. Built with AddressSanitizer enabled to catch use-after-free/overflow bugs the assertions alone wouldn't.
- **Benchmarking against the default allocator** (`benchTest.cpp`, Google Benchmark) — using `DoNotOptimize`/`ClobberMemory` to stop the compiler from eliminating the allocate/free calls being measured, which a naive hand-rolled timing loop under `-O2` would be vulnerable to. Deliberately built as a separate CMake target from the test suite (`-O2`, no sanitizer) since benchmarking and ASan-based correctness testing want opposite compiler settings.

---

# My Extensions and Adaptations

The IBM tutorial provided the foundation for the allocator designs, including implementations of both the basic pool and bitmap approaches, as well as discussion of thread-safe allocation and array/contiguous memory allocation, but lacked the full implementation details due to broken links to the source files. I used these designs as a starting point and independently implemented, adapted, integrated, tested, and evaluated them as a complete C++ project.

My additions and adaptations include:

- Created a reusable header-based interface for integrating the allocators into a C++ project.
- Integrated the allocators with custom `new`, `new[]`, `delete`, and `delete[]` operators.
- Implemented explicit object destruction and array lifetime management around the custom allocation operators.
- Independently implemented the array allocation and contiguous block tracking concepts outlined by the tutorial, whose referenced source implementations were no longer available.
- Added metadata for tracking array allocations and recycling previously freed array regions.
- Built Google Test coverage across all allocator variants.
- Added multithreaded correctness tests to validate the thread-safe implementations.
- Integrated AddressSanitizer into the test configuration to validate memory safety.
- Added Google Benchmark performance measurements and compared the custom allocators against standard `new`/`delete`.
- Identified potential improvements to the array allocation system, including free-region coalescing, reduced internal fragmentation, and multi-chunk allocations.

---

# Results

Benchmarked with Google Benchmark, built at `-O2`, one allocate+construct followed by one destruct+free per operation, `Complex` as the allocated type. Machine-dependent numbers below (Apple Silicon); see [Lessons Learned](#c-concepts-explored) for benchmarking methodology notes.

| Allocator                        | Time per op (ns) | vs. default `new`/`delete` |
|-----------------------------------|------------------:|------------------:|
| Default `new`/`delete`            | 13.8              | 1.0x (baseline)    |
| Basic Pool (single-threaded)      | 3.7               | ~3.7x faster       |
| Basic Pool (thread-safe)          | 18.0              | ~1.3x slower       |
| Bit Map (single-threaded)         | 25.5              | ~1.8x slower       |
| Bit Map (thread-safe)             | 35.2              | ~2.5x slower       |

The single-threaded Basic Pool allocator is the standout: minimal per-block overhead, no locking, and no per-block metadata beyond the recycled `FreeStore` pointer itself, at ~3.7x the speed of the general-purpose default allocator. Since it isn't built for arrays and is scoped to one type per instance, it's best suited to high-churn, single-type, single-threaded allocation patterns — short-lived VFX/particle objects, projectiles in a bullet-hell-style game, or short-lived network packet buffers, for example.

Both bitmap and thread-safe variants come out slower than the default allocator here, largely from mutex-locking overhead (comparing Basic Pool to its thread-safe variant shows roughly a 5x cost from locking alone) and, for the bitmap allocator, bit-scanning cost per allocation. Note the comparison isn't fully apples-to-apples: the default allocator is already thread-safe, while the plain `BasicPoolAllocator`/`BitMapAllocator` are not — the fair comparison against the default allocator is really against the `TS` variants. These pooled allocators would likely close the gap or win outright on a system where requesting memory from the OS is comparatively more expensive than it is here, since caching larger chunks up front is exactly the cost they're designed to amortize.

---

# C++ Concepts Explored

### Memory and Object Lifetime

- `operator delete`/`operator delete[]` overloading requires unique syntax, isn't invoked automatically outside of placement-new construction failure, and never calls the destructor for you
- Why checking `malloc`'s return for null is closer to a hard requirement in some domains (embedded/safety-critical) and a much softer convention on modern overcommitting OSes
- The differences between array guard bytes/cookies and explicit length-tracking for recovering an array's size from a pointer
- Pointer arithmetic depending on the pointed-to type's size
- `union` members

### Language Semantics and Type System

- `static_cast` vs `reinterpret_cast`, and which one is actually correct for `void*` ↔ `T*` conversions
- `std::string`'s small-string optimization limit (heap allocation kicks in past a implementation-defined length, e.g. 22-24 characters on common implementations) and why this makes `std::string` unusable as a `constexpr` value
- `const` vs `constexpr`
- lvalues vs rvalues, and how that shapes call signatures like `DoNotOptimize`
- `virtual`/`override` best practices, and why a base class used polymorphically needs a virtual destructor
- Default constructor behavior with polymorphism, and when base-class initialization is automatic vs. mandatory
- Nested generics and `value_type`

### Compilation and Build System

- Inlining and `.inl` files, and how template inlining differs from ordinary inline functions in the compilation pipeline
- `#pragma once` and its role in the compilation pipeline
- `-O0` vs `-O2` vs `-O3` compilation flags, and why measuring code under `-O0` can misrepresent real-world performance
- CMake project structure, multiple targets, and `find_package`/`target_link_libraries` propagation
- Makefile vs. Ninja as CMake generators

### Testing and Performance Tooling

- Using AddressSanitizer for correctness testing, and why it's built as a separate, differently-optimized target from the benchmarks
- Using Google Benchmark, including avoiding dead-code elimination of the code being measured
- Using GoogleTest, including typed tests to run one test body across multiple allocator types

### Data Structures and Low-Level Behavior

- `size_t`, integer width history, and why `size_t` was never viable as `unsigned int` on 64-bit systems
- Why holding raw pointers into a `std::vector` is fragile, since growth can reallocate and invalidate them

---

# Future Improvements

Several areas could be explored to make the allocators more practical:

### Memory Management

- Allocate entire pools/chunks in a single underlying allocation for the basic pool allocator.
- Improve alignment handling.
- Add configurable pool/chunk sizes.
- Investigate larger allocation support.

### Array Allocation

- Coalesce adjacent freed array regions.
- Track the size of free regions more efficiently.
- Reduce internal fragmentation when reusing larger regions.
- Allow arrays to span multiple chunks.
- Support arrays larger than a single chunk.

### Allocation Performance

- Improve bitmap free-block discovery using more efficient bit operations.
- Investigate alternative data structures for tracking partially free chunks.
- Profile allocator hot paths to identify remaining sources of overhead.
- Benchmark additional object sizes and workload patterns.

### Concurrency

- Investigate finer-grained synchronization.
- Compare mutex-based synchronization with lock-free or atomic approaches.
- Benchmark contention under different numbers of worker threads.
