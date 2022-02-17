#include "opalloc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
* Useful macros
*******************************************************************************/

#define UNUSED(X) ((void)(X))

/*******************************************************************************
* Static helper function declarations
*******************************************************************************/

static bool fill_chunks(op_allocator allocator, const size_t offset, const size_t object_count);
static bool grow_pool(op_allocator allocator);

/*******************************************************************************
* Opaque data structures
*******************************************************************************/

typedef enum _active
{
    NOT_IN_USE,
    IN_USE,
} _active;

typedef struct _ab_t
{
    _active in_use;
    uint8_t data[];
} _ab_t;

struct _op_allocator
{
    size_t object_size;
    size_t initial_count;
    size_t maximum_objects;
    bool   use_chunks;
    bool   use_linear;
    bool   initialized;
    _ab_t   **pool;
};

/*******************************************************************************
* Low-level API function definitions
*******************************************************************************/

void *op_ll_allocate_object(op_allocator allocator)
{
    void *rv = NULL;

    if (allocator && allocator->initialized)
    {
retry:
        for (size_t i = 0; i < allocator->maximum_objects; i++)
        {
            if (allocator->pool[i] == NULL)
            {
                allocator->pool[i] = calloc(1, sizeof(_ab_t) + allocator->object_size);
                if (allocator->pool[i])
                {
                    rv = allocator->pool[i]->data;
                    memset(rv, 0, allocator->object_size);
                    allocator->pool[i]->in_use = IN_USE;
                    break;
                }
                else
                {
                    op_error_handler(__FILE__, __LINE__, "Could not allocate desired object.");
                }
            }
            else if (allocator->pool[i]->in_use == NOT_IN_USE)
            {
                rv = allocator->pool[i]->data;
                memset(rv, 0, allocator->object_size);
                allocator->pool[i]->in_use = IN_USE;
                break;
            }
        }

        if (rv == NULL)
        {
            if (grow_pool(allocator))
            {
                /* This is safe.  If grow_pool() returns true, we are guaranteed there is space, so it won't loop. */
                goto retry;
            }
            else
            {
                op_error_handler(__FILE__, __LINE__, "Unable to grow allocation pool.");
            }
        }
    }
    else
    {
        op_error_handler(__FILE__, __LINE__, "Attempted to allocate from uninitialized allocator.");
    }

    if (rv == NULL)
    {
        op_error_handler(__FILE__, __LINE__, "Could not allocate desired object.");
    }

    return rv;
}

void op_ll_deallocate_object(const op_allocator allocator, const void *object)
{
    if (allocator != NULL && object != NULL)
    {
        for (size_t i = 0; i < allocator->maximum_objects; i++)
        {
            if (allocator->pool[i]->data == object)
            {
                allocator->pool[i]->in_use = NOT_IN_USE;
                return;
            }
        }
    }
    else
    {
        op_error_handler(__FILE__, __LINE__, "Invalid allocator or object while deallocating.");
    }
}

void op_ll_deinitialize_allocator(op_allocator allocator)
{
    if (allocator && allocator->initialized)
    {
        allocator->initialized = false;
        if (allocator->use_chunks)
        {
            if (allocator->use_linear)
            {
                /* this allocator is chunked and uses groups of initial_count allocations */
                for (size_t i = 0; i < allocator->maximum_objects; i += allocator->initial_count)
                {
                    free(allocator->pool[i]);
                }
            }
            else
            {
                /* this allocator is chunked and uses the doubling allocation scheme */
                free(allocator->pool[0]);   /* delete the first chunk of initial_count */
                /* each subsequent chunk starts at doublings from initial_count */
                for (size_t i = allocator->initial_count; i < allocator->maximum_objects; i <<= 1)
                {
                    free(allocator->pool[i]);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < allocator->maximum_objects; i++)
            {
                free(allocator->pool[i]);   /* it's OK to free a NULL, and we initialize to NULL */
            }
        }
        free(allocator->pool);
        free(allocator);
    }
    else
    {
        op_error_handler(__FILE__, __LINE__, "Attempted to deinitialize an allocator that was not initialized.");
    }
}

op_allocator_stats op_ll_get_allocator_stats(const op_allocator allocator)
{
    op_allocator_stats rv = { 0 };
    if (allocator != NULL && allocator->initialized)
    {
        rv.object_size = allocator->object_size;
        rv.maximum_objects = allocator->maximum_objects;
        rv.active_objects = 0;
        for (size_t i = 0; i < allocator->maximum_objects && allocator->pool[i] != NULL; i++)
        {
            if (allocator->pool[i]->in_use == IN_USE) { rv.active_objects++; }
        }
    }
    return rv;
}

op_allocator op_ll_initialize_allocator(const size_t object_size, const size_t initial_count,
                                        const op_ll_allocator_mode mode)
{
    bool use_chunks = false, use_linear = false;
    switch (mode)
    {
    case OP_DOUBLING_INDIVIDUAL:
        use_chunks = false;
        use_linear = false;
        break;

    case OP_DOUBLING_CHUNK:
        use_chunks = true;
        use_linear = false;
        break;

    case OP_LINEAR_INDIVIDUAL:
        use_chunks = false;
        use_linear = true;
        break;

    case OP_LINEAR_CHUNK:
        use_chunks = true;
        use_linear = true;
        break;
    };

    op_allocator rv = calloc(1, sizeof(struct _op_allocator));
    if (rv)
    {
        /* set the allocator configuration */
        rv->object_size = object_size;
        rv->initial_count = rv->maximum_objects = initial_count;
        rv->use_chunks = use_chunks;
        rv->use_linear = use_linear;
        rv->initialized = true;

        /* allocate the space for the object pool itself. */
        if ((rv->pool = calloc(rv->maximum_objects, sizeof(_ab_t *))))
        {
            if (use_chunks)
            {
                if (!fill_chunks(rv, 0, rv->maximum_objects))
                {
                    free(rv->pool);
                    free(rv);
                    rv = NULL;
                }
            }
        }
        else
        {
            free(rv);
            rv = NULL;
            op_error_handler(__FILE__, __LINE__, "Could not allocate internal space for allocator.");
        }
    }
    else
    {
        op_error_handler(__FILE__, __LINE__, "Could not allocate space for allocator handle.");
    }

    return rv;
}

/*******************************************************************************
* Static helper function definitions
*******************************************************************************/

static bool fill_chunks(op_allocator allocator, const size_t offset, const size_t object_count)
{
    bool rv = true;

    size_t entry_size = sizeof(_ab_t) + allocator->object_size;
    size_t chunk_size = object_count * entry_size;
    uint8_t *chunk;
    chunk = calloc(object_count, chunk_size);
    if (chunk)
    {
        for (size_t i = 0; i < object_count; i++)
        {
            allocator->pool[i + offset] = (_ab_t *) &chunk[i * entry_size];
        }
    }
    else
    {
        op_error_handler(__FILE__, __LINE__, "Could not fill chunks when initializing or expanding.");
        rv = false;
    }

    return rv;
}

static bool grow_pool(op_allocator allocator)
{
    bool rv = false;

    size_t old_size = allocator->maximum_objects;
    size_t new_size, grow_size;
    if (allocator->use_linear)
    {
        grow_size = allocator->initial_count;
        new_size = old_size + grow_size;
    }
    else
    {
        grow_size = old_size;
        new_size = old_size * 2;
    }

    allocator->pool = realloc(allocator->pool, sizeof(_ab_t *) * new_size);
    if (allocator->pool)
    {
        /* we use the for loop here because valgrind is too stupid to figure out memset */
        for (size_t i = old_size; i < new_size; i++)
        {
            allocator->pool[i] = NULL;
        }
        allocator->maximum_objects = new_size;
        if (allocator->use_chunks)
        {
            rv = fill_chunks(allocator, old_size, grow_size);
        }
        else
        {
            rv = true;
        }
    }
    else
    {
        op_error_handler(__FILE__, __LINE__, "Could not realloc while growing pool.");
    }

    return rv;
}

/*******************************************************************************
* Weakly-linked function implementations.
*******************************************************************************/

__attribute((weak)) void op_error_handler(const char *file, const int line, const char *error_message)
{
    UNUSED(file);
    UNUSED(line);
    UNUSED(error_message);
}

/*******************************************************************************
* Concealed function used for debugging purposes only.
*******************************************************************************/

#if !defined(__ARM_EABI__)
#include <stdio.h>
void dump_allocator(op_allocator allocator)
{
    fprintf(stderr, "object_size = %lu\n", allocator->object_size);
    fprintf(stderr, "initial_count = %lu\n", allocator->initial_count);
    fprintf(stderr, "maximum_objects = %lu\n", allocator->maximum_objects);
    fprintf(stderr, "use_chunks = %d\n", allocator->use_chunks);
    fprintf(stderr, "use_linear = %d\n", allocator->use_linear);
    fprintf(stderr, "initialized = %d\n", allocator->initialized);
    fprintf(stderr, "pool = %p\n", allocator->pool);
    for (size_t i = 0; i < allocator->maximum_objects; i++)
    {
        _ab_t *ab = allocator->pool[i];
        if (ab != NULL)
        {
            fprintf(stderr, "\t%lu - %p -> %d %p\n", i, allocator->pool[i], allocator->pool[i]->in_use, allocator->pool[i]->data);
        }
        else
        {
            fprintf(stderr, "\t%lu - NULL\n", i);
        }
    }
}
#endif
