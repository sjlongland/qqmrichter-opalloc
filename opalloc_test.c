#include "opalloc.h"

#include <assert.h>
#include <stdio.h>

#define MINIMUM_ALLOCATION_COUNT 4
#define UNUSED(X)                ((void)(X))


extern const char PROJECT_NAME[], PROJECT_OID_DOTTED[];

extern void dump_allocator(op_allocator allocator);

typedef struct test_object
{
    bool running;
    int stack_size;
} test_object;
typedef void (*test_func)(void);

/*
 * Multiple allocators can be made.  Allocators clean up properly.
 */
static void ll_test1(void)
{
    for (size_t size = MINIMUM_ALLOCATION_COUNT; size < 1000; size += 17)
    {
        op_allocator allocator1 = op_ll_initialize_allocator(sizeof(test_object), size, OP_DOUBLING_INDIVIDUAL);
        op_allocator allocator2 = op_ll_initialize_allocator(sizeof(test_object), size, OP_DOUBLING_CHUNK);
        op_allocator allocator3 = op_ll_initialize_allocator(sizeof(test_object), size, OP_LINEAR_INDIVIDUAL);
        op_allocator allocator4 = op_ll_initialize_allocator(sizeof(test_object), size, OP_LINEAR_CHUNK);

        op_ll_deinitialize_allocator(allocator1);
        op_ll_deinitialize_allocator(allocator2);
        op_ll_deinitialize_allocator(allocator3);
        op_ll_deinitialize_allocator(allocator4);
    }
}

/*
 * Multiple items can be made from an allocator.  Allocator cleans up properly.
 * No unnecessary growth happens.
 */
static void ll_test2(void)
{
    op_allocator allocator1 = op_ll_initialize_allocator(sizeof(test_object), MINIMUM_ALLOCATION_COUNT, OP_DOUBLING_INDIVIDUAL);

    test_object *item1a = op_ll_allocate_object(allocator1);
    assert(item1a != NULL);
    assert(item1a->running == false);
    assert(item1a->stack_size == 0);

    test_object *item1b = op_ll_allocate_object(allocator1);
    assert(item1b != NULL);
    assert(item1a != item1b);
    assert(item1b->running == false);
    assert(item1b->stack_size == 0);

    test_object *item1c = op_ll_allocate_object(allocator1);
    assert(item1c != NULL);
    assert(item1c != item1b);
    assert(item1c != item1a);
    assert(item1c->running == false);
    assert(item1c->stack_size == 0);

    op_allocator_stats stats = op_ll_get_allocator_stats(allocator1);
    assert(stats.maximum_objects == 4);
    assert(stats.object_size == sizeof(test_object));
    assert(stats.active_objects == 3);

    op_ll_deinitialize_allocator(allocator1);
}

/*
 * Allocator which overflows doubles in size.  Allocator cleans up properly.
 */
static void ll_test3(void)
{
    op_allocator allocator1 = op_ll_initialize_allocator(sizeof(test_object), MINIMUM_ALLOCATION_COUNT, OP_DOUBLING_INDIVIDUAL);

    test_object *item1a = op_ll_allocate_object(allocator1);
    test_object *item1b = op_ll_allocate_object(allocator1);
    test_object *item1c = op_ll_allocate_object(allocator1);
    test_object *item1d = op_ll_allocate_object(allocator1);

    test_object *item1e = op_ll_allocate_object(allocator1);
    assert(item1e != NULL);
    assert(item1e->running == false);
    assert(item1e->stack_size == 0);

    op_allocator_stats stats = op_ll_get_allocator_stats(allocator1);
    assert(stats.maximum_objects == 8);
    assert(stats.object_size == sizeof(test_object));
    assert(stats.active_objects == 5);

    op_ll_deinitialize_allocator(allocator1);

    UNUSED(item1a);
    UNUSED(item1b);
    UNUSED(item1c);
    UNUSED(item1d);
}

/*
 * Allocator doubles twice.  Allocator cleans up.
 */
static void ll_test4(void)
{
    op_allocator allocator1 = op_ll_initialize_allocator(sizeof(test_object), MINIMUM_ALLOCATION_COUNT, OP_DOUBLING_INDIVIDUAL);

    test_object *item1a = op_ll_allocate_object(allocator1);
    test_object *item1b = op_ll_allocate_object(allocator1);
    test_object *item1c = op_ll_allocate_object(allocator1);
    test_object *item1d = op_ll_allocate_object(allocator1);
    test_object *item1e = op_ll_allocate_object(allocator1);
    test_object *item1f = op_ll_allocate_object(allocator1);
    test_object *item1g = op_ll_allocate_object(allocator1);
    test_object *item1h = op_ll_allocate_object(allocator1);

    test_object *item1i = op_ll_allocate_object(allocator1);
    assert(item1i != NULL);
    assert(item1i->running == false);
    assert(item1i->stack_size == 0);

    op_allocator_stats stats = op_ll_get_allocator_stats(allocator1);
    assert(stats.maximum_objects == 16);
    assert(stats.object_size == sizeof(test_object));
    assert(stats.active_objects == 9);

    op_ll_deinitialize_allocator(allocator1);

    UNUSED(item1a);
    UNUSED(item1b);
    UNUSED(item1c);
    UNUSED(item1d);
    UNUSED(item1e);
    UNUSED(item1f);
    UNUSED(item1g);
    UNUSED(item1h);
}

/*
 * Objects can be deleted from allocator.  Deleted objects are reused without growing allocator.
 * Allocator cleans up properly.
 */
static void ll_test5(void)
{
    op_allocator allocator1 = op_ll_initialize_allocator(sizeof(test_object), MINIMUM_ALLOCATION_COUNT, OP_DOUBLING_INDIVIDUAL);

    test_object *item1a = op_ll_allocate_object(allocator1);
    test_object *item1b = op_ll_allocate_object(allocator1);
    test_object *item1c = op_ll_allocate_object(allocator1);
    test_object *item1d = op_ll_allocate_object(allocator1);

    test_object *item1e = op_ll_allocate_object(allocator1);
    test_object *item1f = op_ll_allocate_object(allocator1);
    test_object *item1g = op_ll_allocate_object(allocator1);
    test_object *item1h = op_ll_allocate_object(allocator1);

    op_ll_deallocate_object(allocator1, item1c);
    op_ll_deallocate_object(allocator1, item1g);

    test_object *item1i = op_ll_allocate_object(allocator1);
    assert(item1i != NULL);
    assert(item1i->running == false);
    assert(item1i->stack_size == 0);

    op_allocator_stats stats = op_ll_get_allocator_stats(allocator1);
    assert(stats.maximum_objects == 8);
    assert(stats.object_size == sizeof(test_object));
    assert(stats.active_objects == 7);

    op_ll_deinitialize_allocator(allocator1);

    UNUSED(item1a);
    UNUSED(item1b);
    UNUSED(item1d);
    UNUSED(item1e);
    UNUSED(item1f);
    UNUSED(item1h);
}

/*
 * Linear growth grows by block sizes established at creation.
 * Linearly-grown allocator cleans up properly.
 */
static void ll_test6(void)
{
    op_allocator allocator1 = op_ll_initialize_allocator(sizeof(test_object), MINIMUM_ALLOCATION_COUNT, OP_LINEAR_INDIVIDUAL);

    test_object *item1a = op_ll_allocate_object(allocator1);
    test_object *item1b = op_ll_allocate_object(allocator1);
    test_object *item1c = op_ll_allocate_object(allocator1);
    test_object *item1d = op_ll_allocate_object(allocator1);
    test_object *item1e = op_ll_allocate_object(allocator1);
    test_object *item1f = op_ll_allocate_object(allocator1);
    test_object *item1g = op_ll_allocate_object(allocator1);
    test_object *item1h = op_ll_allocate_object(allocator1);
    test_object *item1i = op_ll_allocate_object(allocator1);

    op_ll_deallocate_object(allocator1, item1c);
    op_ll_deallocate_object(allocator1, item1g);

    test_object *item1j = op_ll_allocate_object(allocator1);

    assert(item1j != NULL);
    assert(item1j->running == false);
    assert(item1j->stack_size == 0);

    op_allocator_stats stats = op_ll_get_allocator_stats(allocator1);
    assert(stats.maximum_objects == 12);
    assert(stats.object_size == sizeof(test_object));
    assert(stats.active_objects == 8);

    op_ll_deinitialize_allocator(allocator1);

    UNUSED(item1a);
    UNUSED(item1b);
    UNUSED(item1d);
    UNUSED(item1e);
    UNUSED(item1f);
    UNUSED(item1h);
    UNUSED(item1i);
}

/*
 * Doubling growth.
 * Chunk allocation works as expected.
 * Allocator cleans up properly.
 */
static void ll_test7(void)
{
    op_allocator allocator1 = op_ll_initialize_allocator(sizeof(test_object), MINIMUM_ALLOCATION_COUNT, OP_DOUBLING_CHUNK);

    test_object *item1a = op_ll_allocate_object(allocator1);
    test_object *item1b = op_ll_allocate_object(allocator1);
    test_object *item1c = op_ll_allocate_object(allocator1);
    test_object *item1d = op_ll_allocate_object(allocator1);
    test_object *item1e = op_ll_allocate_object(allocator1);
    test_object *item1f = op_ll_allocate_object(allocator1);
    test_object *item1g = op_ll_allocate_object(allocator1);
    test_object *item1h = op_ll_allocate_object(allocator1);
    test_object *item1i = op_ll_allocate_object(allocator1);

    op_ll_deallocate_object(allocator1, item1c);
    op_ll_deallocate_object(allocator1, item1g);

    test_object *item1j = op_ll_allocate_object(allocator1);

    assert(item1j != NULL);
    assert(item1j->running == false);
    assert(item1j->stack_size == 0);

    op_allocator_stats stats = op_ll_get_allocator_stats(allocator1);
    assert(stats.maximum_objects == 16);
    assert(stats.object_size == sizeof(test_object));
    assert(stats.active_objects == 8);

    op_ll_deinitialize_allocator(allocator1);

    UNUSED(item1a);
    UNUSED(item1b);
    UNUSED(item1d);
    UNUSED(item1e);
    UNUSED(item1f);
    UNUSED(item1h);
    UNUSED(item1i);
}

/*
 * Linear growth grows by block sizes established at creation.
 * Chunk allocation works as expected.
 * Allocator cleans up properly.
 */
static void ll_test8(void)
{
    op_allocator allocator1 = op_ll_initialize_allocator(sizeof(test_object), MINIMUM_ALLOCATION_COUNT, OP_LINEAR_CHUNK);

    test_object *item1a = op_ll_allocate_object(allocator1);
    test_object *item1b = op_ll_allocate_object(allocator1);
    test_object *item1c = op_ll_allocate_object(allocator1);
    test_object *item1d = op_ll_allocate_object(allocator1);
    test_object *item1e = op_ll_allocate_object(allocator1);
    test_object *item1f = op_ll_allocate_object(allocator1);
    test_object *item1g = op_ll_allocate_object(allocator1);
    test_object *item1h = op_ll_allocate_object(allocator1);
    test_object *item1i = op_ll_allocate_object(allocator1);

    op_ll_deallocate_object(allocator1, item1c);
    op_ll_deallocate_object(allocator1, item1g);

    test_object *item1j = op_ll_allocate_object(allocator1);

    assert(item1j != NULL);
    assert(item1j->running == false);
    assert(item1j->stack_size == 0);

    op_allocator_stats stats = op_ll_get_allocator_stats(allocator1);
    assert(stats.maximum_objects == 12);
    assert(stats.object_size == sizeof(test_object));
    assert(stats.active_objects == 8);

    op_ll_deinitialize_allocator(allocator1);

    UNUSED(item1a);
    UNUSED(item1b);
    UNUSED(item1d);
    UNUSED(item1e);
    UNUSED(item1f);
    UNUSED(item1h);
    UNUSED(item1i);
}

/* declare a type-safe allocator suite */
OP_HL_DECLARE_ALLOCATOR(test_object, MINIMUM_ALLOCATION_COUNT, OP_DOUBLING_CHUNK);

/*
 * Test as per ll_test6().
 */
void hl_test1(void)
{
    test_object *item1a = allocate_test_object(); // automatically initializes the allocator
    test_object *item1b = allocate_test_object();
    test_object *item1c = allocate_test_object();
    test_object *item1d = allocate_test_object();
    test_object *item1e = allocate_test_object();
    test_object *item1f = allocate_test_object();
    test_object *item1g = allocate_test_object();
    test_object *item1h = allocate_test_object();
    test_object *item1i = allocate_test_object();

    deallocate_test_object(item1c);
    deallocate_test_object(item1g);

    test_object *item1j = allocate_test_object();

    assert(item1j != NULL);
    assert(item1j->running == false);
    assert(item1j->stack_size == 0);

    op_allocator_stats stats = op_ll_get_allocator_stats(test_object_allocator);
    assert(stats.maximum_objects == 16);
    assert(stats.object_size == sizeof(test_object));
    assert(stats.active_objects == 8);

    UNUSED(item1a);
    UNUSED(item1b);
    UNUSED(item1d);
    UNUSED(item1e);
    UNUSED(item1f);
    UNUSED(item1h);
    UNUSED(item1i);
}

static test_func ll_tests[] =
{
    ll_test1, ll_test2, ll_test3, ll_test4, ll_test5, ll_test6, ll_test7, ll_test8,
    NULL,
};

static test_func hl_tests[] =
{
    hl_test1,
    NULL,
};


int main(int argc, char** argv)
{
    fprintf(stdout, "%s (%s)\n", PROJECT_NAME, PROJECT_OID_DOTTED);

    /* low-level API tests */
    fprintf(stdout, "Low-level tests: ");
    for (size_t i = 0; ll_tests[i] != NULL; i++)
    {
        fprintf(stdout, "%lu ", i + 1); fflush(stdout);
        ll_tests[i]();
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "High-level tests: ");
    for (size_t i = 0; hl_tests[i] != NULL; i++)
    {
        fprintf(stdout, "%lu ", i + 1); fflush(stdout);
        hl_tests[i]();
    }
    fprintf(stdout, "\n");
    deinitialize_test_object_allocator();

    return 0;
}

void op_error_handler(const char *file, const int line, const char* error_message)
{
    fprintf(stderr, "ERROR: %s (%d): %s\n", file, line, error_message);
}
