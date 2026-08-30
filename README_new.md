# Memory Allocator

A from-scratch C++ memory allocator library implementing two pooled allocation strategies, generalized to arbitrary types, made thread-safe, benchmarked against the default `new`/`delete`, and covered by a full GoogleTest suite with AddressSanitizer.

This project started from the 2008 IBM Developer tutorial ["Writing a custom memory manager"](https://developer.ibm.com/tutorials/au-memorymanager/) by Arpan Sen and Rahul Kardam, which covers two hardcoded, single-purpose allocator designs. Everything past that starting point — templating both allocators for any type, adding thread-safe variants, building a portable single-header integration, testing, and benchmarking — is my own work, detailed below.

## Contents
- [The Allocators](#the-allocators)
- [Beyond the Tutorial](#beyond-the-tutorial)
- [Results](#results)
- [Lessons Learned](#lessons-learned)
- [Possible Improvements](#possible-improvements)

## The Allocators

Both allocators take the same basic approach: request large pools of memory from the OS up front, and recycle blocks out of that pool internally instead of going back to the OS on every allocation/free.

### Basic Pool Allocator

A linked-list-of-fixed-size-blocks allocator. Each `BasicPoolAllocator<T>` instance manages blocks sized for exactly one type `T`.

Free blocks are recast in place as a `FreeStore` node, and the allocator only tracks the head of this list — meaning the pool's own currently-unused memory doubles as the free-list's metadata storage, with no separate bookkeeping structure needed.

- **Allocate**: return the current head block, and advance the head to whatever that block's `FreeStore::next` points to. If there is no next block, expand the pool first.
- **Free**: recast the freed block back to a `FreeStore` node, point its `next` at the current head, and make it the new head.

Because freed blocks are pushed onto the front of the list, the list's order drifts away from address order over the pool's lifetime — this is expected, not a bug.

Known limitations:
- No array support — arrays fall back to the OS allocator.
- One allocator instance per type (by design — see [Beyond the Tutorial](#beyond-the-tutorial)).
- Each block must be at least `sizeof(FreeStore)`, which wastes space for types smaller than a pointer.

### Bit Map Pool Allocator

Also pool-based, but tracks free/occupied blocks with a bitmap (an array of ints, one bit per block) instead of a linked list.

Growing the pool allocates a "chunk" from the OS and divides it into equal-sized blocks for the allocator's type. Each chunk gets a `BitMapEntry` holding its bitmap (initialized to all-1s, meaning free) plus helper functions to find and flip bits.

- **Allocate**: find the first free bit across all chunks, flip it to occupied, and return that block's address cast to the target type.
- **Free**: flip the block's bit back to free. The memory itself is left untouched until reused.

Array support is handled separately: dedicated chunks are reserved just for array allocations, tracked via a map from array head pointer to an `ArrayMemoryInfo` (length + chunk offset). Because the bitmap guarantees block order matches OS memory order within a chunk, arrays can be allocated as a single run of contiguous blocks. Freed arrays are tracked for reuse by future array allocations that fit in the freed space.

Known limitations (see [Possible Improvements](#possible-improvements) for the fixes):
- A new array allocation reusing a larger freed array's space wastes the leftover blocks.
- Freed arrays aren't merged with adjacent free arrays, so chunks can fragment into unusable gaps.
- Arrays can't span multiple chunks or exceed one chunk's size — a new chunk is always allocated instead.

## Beyond the Tutorial

The IBM tutorial implements one non-generic, single-threaded allocator per strategy, hardcoded to one class (`Complex`), with no discussion of `delete`/`delete[]` overload semantics. Extending it into something usable required:

- **Generic types via templates.** Both allocators are `template<class T>` classes (`BasicPoolAllocator<T>`, `BitMapAllocator<T>`) instead of being hardcoded to one type — every size comparison and cast in the tutorial's version is baked in for `Complex` specifically. Templating means one allocator implementation now serves any type, the same way `std::vector<T>` does, without touching the underlying allocation logic.
- **Thread safety.** `BasicPoolTSAllocator<T>` and `BitMapTSAllocator<T>` wrap the base allocators with mutex locking around allocate/free, so both a fast single-threaded and a safe multi-threaded variant of each strategy are available. The tutorial doesn't address concurrency at all.
- **A common interface.** All four allocators implement `IMemoryAllocator`, a small type-erased interface (`allocate`/`free`/`allocateArray`/`freeArray`), so calling code can work against any allocator polymorphically.
- **A portable, single-header integration (`allocators/customMemoryAllocator.h`).** Global placement `operator new`/`operator new[]` overloads taking an `IMemoryAllocator&`, so usage looks like ordinary C++: `new (allocator) T(args...)`.
- **Correct, documented `delete`/`delete[]` overloads — the tutorial's biggest gap.** A custom `operator delete` with an extra allocator parameter is *not* automatically invoked by `delete` the way the built-in one is, and unlike the built-in operator, it does not call the destructor for you — both have to be done manually inside the overload. This is easy to get wrong silently: `operator delete[]` additionally can't infer how many elements it's destructing from the pointer alone, since there's no portable way to recover an array's length from a raw pointer. My overload takes an explicit length parameter (`operator delete<T>[](p, allocator, length)`) rather than relying on compiler-specific ABI array-cookie tricks, so it destructs exactly the right number of elements regardless of implementation details the standard doesn't guarantee.
- **A custom exception type (`BadAllocWithMsg`)** thrown on allocation failure, so failures are reported explicitly and distinguishably rather than silently returning null or aborting.
- **A full test suite** (`AllocatorTests.cpp`, GoogleTest) — typed tests that run the same allocate/free/array/oversized-request/reuse checks across all four allocator types via `TYPED_TEST_SUITE`, plus a dedicated multi-threaded stress test hammering the two thread-safe allocators concurrently and checking for address collisions. Built with AddressSanitizer enabled to catch use-after-free/overflow bugs the assertions alone wouldn't.
- **Benchmarking against the default allocator** (`benchTest.cpp`, Google Benchmark) — using `DoNotOptimize`/`ClobberMemory` to stop the compiler from eliminating the allocate/free calls being measured, which a naive hand-rolled timing loop under `-O2` would be vulnerable to. Deliberately built as a separate CMake target from the test suite (`-O2`, no sanitizer) since benchmarking and ASan-based correctness testing want opposite compiler settings.

## Results

Benchmarked with Google Benchmark, built at `-O2`, one allocate+construct followed by one destruct+free per operation, `Complex` as the allocated type. Machine-dependent numbers below (Apple Silicon); see [Lessons Learned](#lessons-learned) for benchmarking methodology notes.

| Allocator                        | Time per op (ns) | vs. default `new`/`delete` |
|-----------------------------------|------------------:|------------------:|
| Default `new`/`delete`            | 13.8              | 1.0x (baseline)    |
| Basic Pool (single-threaded)      | 3.7               | ~3.7x faster       |
| Basic Pool (thread-safe)          | 18.0              | ~1.3x slower       |
| Bit Map (single-threaded)         | 25.5              | ~1.8x slower       |
| Bit Map (thread-safe)             | 35.2              | ~2.5x slower       |

The single-threaded Basic Pool allocator is the standout: minimal per-block overhead, no locking, and no per-block metadata beyond the recycled `FreeStore` pointer itself, at ~3.7x the speed of the general-purpose default allocator. Since it isn't built for arrays and is scoped to one type per instance, it's best suited to high-churn, single-type, single-threaded allocation patterns — short-lived VFX/particle objects, projectiles in a bullet-hell-style game, or short-lived network packet buffers, for example.

Both bitmap and thread-safe variants come out slower than the default allocator here, largely from mutex-locking overhead (comparing Basic Pool to its thread-safe variant shows roughly a 5x cost from locking alone) and, for the bitmap allocator, bit-scanning cost per allocation. Note the comparison isn't fully apples-to-apples: the default allocator is already thread-safe, while the plain `BasicPoolAllocator`/`BitMapAllocator` are not — the fair comparison against the default allocator is really against the `TS` variants. These pooled allocators would likely close the gap or win outright on a system where requesting memory from the OS is comparatively more expensive than it is here, since caching larger chunks up front is exactly the cost they're designed to amortize.

## Lessons Learned

- `operator delete`/`operator delete[]` overloading requires unique syntax, isn't invoked automatically outside of placement-new construction failure, and never calls the destructor for you
- `std::string`'s small-string optimization limit (heap allocation kicks in past a implementation-defined length, e.g. 22-24 characters on common implementations) and why this makes `std::string` unusable as a `constexpr` value
- `static_cast` vs `reinterpret_cast`, and which one is actually correct for `void*` ↔ `T*` conversions
- `virtual`/`override` best practices, and why a base class used polymorphically needs a virtual destructor
- Inlining and `.inl` files, and how template inlining differs from ordinary inline functions in the compilation pipeline
- lvalues vs rvalues, and how that shapes call signatures like `DoNotOptimize`
- Why checking `malloc`'s return for null is closer to a hard requirement in some domains (embedded/safety-critical) and a much softer convention on modern overcommitting OSes
- Nested generics and `value_type`
- Default constructor behavior with polymorphism, and when base-class initialization is automatic vs. mandatory
- `size_t`, integer width history, and why `size_t` was never viable as `unsigned int` on 64-bit systems
- `const` vs `constexpr`
- Using AddressSanitizer for correctness testing, and why it's built as a separate, differently-optimized target from the benchmarks
- Using Google Benchmark, including avoiding dead-code elimination of the code being measured
- Using GoogleTest, including typed tests to run one test body across multiple allocator types
- CMake project structure, multiple targets, and `find_package`/`target_link_libraries` propagation
- Makefile vs. Ninja as CMake generators
- Why holding raw pointers into a `std::vector` is fragile, since growth can reallocate and invalidate them
- The differences between array guard bytes/cookies and explicit length-tracking for recovering an array's size from a pointer
- Pointer arithmetic depending on the pointed-to type's size
- `#pragma once` and its role in the compilation pipeline
- `union` members
- `-O0` vs `-O2` vs `-O3` compilation flags, and why measuring code under `-O0` can misrepresent real-world performance

## Possible Improvements

- Avoid wasting leftover space when a new array allocation reuses a larger previously-freed array's blocks
- Merge freed arrays with adjacent free arrays to avoid fragmenting chunks into unusable gaps (or otherwise track/communicate gap sizes)
- Allow arrays to span multiple chunks, or exceed a single chunk's size, instead of always allocating a new chunk
