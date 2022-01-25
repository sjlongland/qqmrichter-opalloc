# Object Pool Allocator

An "object pool" framework for minimally-fragmenting interlaced dynamic
allocation and deallocation of dynamic memory objects in C.

In embedded and small-memory systems, excessive use of dynamically allocated
objects can cause heap fragmentation such that, despite there being sufficient
free space in the heap for an object, calls to `malloc()` and related functions
fail because no single block is large enough to fulfill the desired request.

This library provides an implementation of a smarter memory pool manager that
combats this problem by minimizing the number of interlaced allocations,
deallocations and reallocations at a slight expense in memory and CPU overhead.
It supplies a low level interface for building custom allocators in cases where
fine control is needed, a high level interface for the most common and simple
use cases, and a macro interface for the easiest use case.

C.f. https://personaljournal.ca/qqmrichter/dynamically-static-allocation-in-embedded-systems
for details on implementation and rationale.
