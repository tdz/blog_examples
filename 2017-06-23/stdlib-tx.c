/* This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The person who associated a work with this deed has dedicated the
 * work to the public domain by waiving all of his or her rights to
 * the work worldwide under copyright law, including all related and
 * neighboring rights, to the extent allowed by law. You can copy,
 * modify, distribute and perform the work, even for commercial
 * purposes, all without asking permission.
 */

#include "stdlib-tx.h"
#include "tm.h"

static void
undo_malloc_tx(uintptr_t data)
{
    void* ptr = (void*)data;
    free(ptr);
}

void*
malloc_tx(size_t size)
{
    void* ptr = malloc(size);

    append_to_log(NULL, undo_malloc_tx, (uintptr_t)ptr);

    return ptr;
}

static void
apply_free_tx(uintptr_t data)
{
    void* ptr = (void*)data;
    free(ptr);
}

void
free_tx(void* ptr)
{
    append_to_log(apply_free_tx, NULL, (uintptr_t)ptr);
}
