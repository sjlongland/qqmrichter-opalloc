/* vim: ft=c */
#ifndef OPALLOC_INCLUDED
#define OPALLOC_INCLUDED
/******************************************************************************
* (c)2022 Michael T. Richter
*
* This software is distributed under the terms of WTFPLv2.  The full terms and
* text of the license can be found at http://www.wtfpl.net/txt/copying
******************************************************************************/
/** @file
 *  @brief An "object pool" framework for minimally-fragmenting interlaced
 *         dynamic allocation and deallocation of dynamic memory objects in C.
 *
 * In embedded and small-memory systems, excessive use of dynamically
 * allocated objects can cause heap fragmentation such that, despite there
 * being sufficient free space in the heap for an object, calls to `malloc()`
 * and related functions fail because no single block is large enough to
 * fulfill the desired request.
 *
 * This library provides an implementation of a smarter memory pool manager that
 * combats this problem by minimizing the number of interlaced allocations,
 * deallocations and reallocations at a slight expense in memory and CPU
 * overhead.  It supplies a low level interface for building custom allocators
 * in cases where fine control is needed, a high level interface for the most
 * common and simple use cases, and a macro interface for the easiest use case.
 *
 * @see https://personaljournal.ca/qqmrichter/dynamically-static-allocation-in-embedded-systems
 */

#include <stdbool.h>
#include <stddef.h>

/** @defgroup llinterface Low-level interface
 *
 * This is the nuts-and-bolts interface to the library.  It is primarily used to
 * implement the high level interface and the macro interface but has been
 * exposed to enable end-users to make task-specific interfaces if they so
 * choose.
 *
 * It is strongly recommended not to use this interface directly as it is highly
 * error-prone.
 *
 * @{
 */

/** @brief Allocator mode handling. */
typedef enum op_ll_allocator_mode
{
    OP_DOUBLING_INDIVIDUAL, /**< doubling growth, individual object allocation */
    OP_DOUBLING_CHUNK,      /**< doubling growth, chunk object allocation      */
    OP_LINEAR_INDIVIDUAL,   /**< linear growth, individual object allocation   */
    OP_LINEAR_CHUNK,        /**< linear growth, chunk object allocation        */
} op_ll_allocator_mode;

/** @brief Opaque allocator handle permitting multiple object pools. */
typedef struct _op_allocator *op_allocator;

/** @brief Stats for measuring and debugging allocators. */
typedef struct op_allocator_stats
{
    size_t object_size;     /**< size in bytes of stored objects                   */
    size_t maximum_objects; /**< maximum number of objects currently allocated for */
    size_t active_objects;  /**< number of objects actively in use                 */
} op_allocator_stats;

/** @brief Error callback for allocator errors.
 *
 * @param [in] file          The source file where the error was discovered.
 * @param [in] line          The line of the file where the error was discovered.
 * @param [in] error_message The error's textual message.
 *
 * A weakly-linked implementation is provided in case no special error handling
 * is desired.
 */
void op_error_handler(const char *file, const int line, const char* error_message);

/** @brief Allocate an object in the given allocator context.
 *
 * @param [in,out] allocator The allocator from which the memory is to be allocated.
 *
 * @return A pointer to the memory space, NULL on failure.
 *
 * @note The `op_error_handler()` callback is called before NULL is returned.
 */
void *op_ll_allocate_object(op_allocator allocator);

/** @brief Deallocate a specified object from the given allocator.
 *
 * @param [in, out] allocator The allocator holding the object being freed.
 * @param [in]      object    The object to be freed.
 */
void op_ll_deallocate_object(op_allocator allocator, const void *object);

/** @brief De-initialize an allocator, freeing any owned resources.
 *
 * @param [in, out] allocator The allocator from which to free the object.
 *
 * @note After returning, the allocator object is invalid and can no longer be
 *       used.
 */
void op_ll_deinitialize_allocator(op_allocator allocator);

/** @brief De-initialize an allocator, freeing any owned resources.
 *
 * @param [in] allocator The allocator to collect stats from.
 *
 * @return A structure with some allocator statistics for measurement and
 *         debugging.
 */
op_allocator_stats op_ll_get_allocator_stats(const op_allocator allocator);

/** @brief Initialize an allocator according to the provided configuration.
 *
 * @param [in] object_size   The size each individual object requires in RAM.
 * @param [in] initial_count The starting size of the allocator's array.
 * @param [in] use_chunks    Signals this allocator uses chunk allocation.
 * @param [in] use_linear    Signals this allocator uses linear growth.
 *
 * @return An `op_allocator` handle used in subsequent operations or NULL on
 *         failure.
 *
 * @note 1. If `use_chunks` is not provided, only the indirect array is
 *          calloced in the allocator.  If `use_chunks` is provided, the first
 *          `initial_count` objects of `object_size` bytes are also allocated.
 *       2. On failure NULL is returned, but `op_error_handler()` is called
 *          first.
 */
op_allocator op_ll_initialize_allocator(const size_t object_size, const size_t initial_count,
                                        const op_ll_allocator_mode mode);

/**@}*/

/** @defgroup hlinterface High-level (Macro) interface
 *
 * @param [in] TYPE  The type of objects to be allocated.
 * @param [in] COUNT The initial object count for the allocator.
 * @param [in] MODE  The mode of the allocator (as per the low-level interface).
 *
 * The high-level interface is the intended use of this library.  The focus is
 * on a component based design where a single source file contains a single
 * component which is allocated and deallocated dynamically, thus profiting from
 * reuse to minimize fragmentation and increase amortized allocation speed.
 *
 * Each use of the `OP_HL_DECLARE_ALLOCATOR()` macro creates one type-safe
 * allocator using the starting count and mode provided.  It also creates
 * several static functions whose name invoke the given type.  For example:
 *
 *     OP_HL_DECLARE_ALLOCATOR(my_fancy_type, 4, OP_DOUBLING_CHUNK);
 *
 * This invocation results in the following static functions:
 *
 *     static inline void initialize_my_fancy_type_allocator(void);
 *     static inline my_fancy_type *allocate_my_fancy_type(void);
 *     static inline void deallocate_my_fancy_type(my_fancy_type *object);
 *     static inline void deinitialize_my_fancy_type_allocator(void);
 *
 * Low-level operations can still be used if desired, via the provided:
 *
 *     static op_allocator my_fancy_type_allocator;
 *
 * Use of all of these is precisely the same as for the low-level interface, but
 * the allocation and deallocation is type-safe.
 *
 * @note There is no real need to call the initialization function because the
 *       allocation function does that automatically on first invocation.
 *
 * @note The deinitialization function is really only needed for obsessive
 *       clean-up.
 *
 * @{
 */

/** @brief Declare a component allocator.
 *
 */
#define OP_HL_DECLARE_ALLOCATOR(TYPE, COUNT, MODE)                            \
static op_allocator TYPE##_allocator;                                         \
static inline void initialize_##TYPE##_allocator(void)                        \
{ TYPE##_allocator = op_ll_initialize_allocator(sizeof(TYPE), COUNT, MODE); } \
static inline TYPE *allocate_##TYPE(void)                                     \
{ static bool initialized = false; if (!initialized)                          \
  { initialize_##TYPE##_allocator(); initialized = true; }                    \
  return op_ll_allocate_object(TYPE##_allocator); }                           \
static inline void deallocate_##TYPE(TYPE *object)                            \
{ op_ll_deallocate_object(TYPE##_allocator, object); }                        \
static inline void deinitialize_##TYPE##_allocator(void)                      \
{ op_ll_deinitialize_allocator(TYPE##_allocator); }

/**@}*/

#endif
