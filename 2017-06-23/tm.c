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

#include "tm.h"
#include "array.h"
#include "res.h"

void
privatize(uintptr_t addr, size_t siz, bool load, bool store)
{
    while (siz) {

        struct resource* res = acquire_resource(addr & BASE_BITMASK);
        if (!res) {
            tm_restart();
        }

        unsigned long index = addr & RESOURCE_BITMASK;
        unsigned long bits = 1ul << index;

        uint8_t* beg = arraybeg(res->local_value) + index;
        uint8_t* end = arrayend(res->local_value);

        while (siz && (beg < end)) {
            /* If we're about to store, we first have to
             * save the old value for possible rollbacks. */
            if (store && !(res->local_bits & bits) ) {
                *beg = *((uint8_t*)addr);
            }

            bits <<= 1;
            --siz;
            ++addr;
            ++beg;
        }

        if (store) {
            res->flags |= RESOURCE_FLAG_WRITE_THROUGH;
        }
    }
}

void
load(uintptr_t addr, void* buf, size_t siz)
{
    uint8_t* mem = (uint8_t*)buf;

    while (siz) {

        struct resource* res = acquire_resource(addr & BASE_BITMASK);
        if (!res) {
            tm_restart();
        }

        unsigned long index = addr & RESOURCE_BITMASK;
        unsigned long bits = 1ul << index;

        uint8_t* beg = arraybeg(res->local_value) + index;
        uint8_t* end = arrayend(res->local_value);

        while (siz && (beg < end)) {
            if (res->local_bits & bits) {
                *mem = *beg;
            } else {
                *mem = *((uint8_t*)addr);
            }

            bits <<= 1;
            --siz;
            ++addr;
            ++mem;
            ++beg;
        }
    }
}

void
store(uintptr_t addr, const void* buf, size_t siz)
{
    const uint8_t* mem = (const uint8_t*)buf;

    while (siz) {

        struct resource* res = acquire_resource(addr & BASE_BITMASK);
        if (!res) {
            tm_restart();
        }

        unsigned long index = addr & RESOURCE_BITMASK;
        unsigned long bits = 1ul << index;

        uint8_t* beg = arraybeg(res->local_value) + index;
        uint8_t* end = arrayend(res->local_value);

        while (siz && (beg < end)) {
            *beg = *mem;
            res->local_bits |= bits;

            bits <<= 1;
            --siz;
            ++addr;
            ++mem;
            ++beg;
        }
    }
}

int
load_int(const int* addr)
{
    int value;
    load((uintptr_t)addr, &value, sizeof(value));
    return value;
}

void
store_int(int* addr, int value)
{
    store((uintptr_t)addr, &value, sizeof(value));
}

struct _tm_tx*
_tm_get_tx()
{
    /* Thread-local transaction structure */
    static __thread struct _tm_tx t_tm_tx;

    return &t_tm_tx;
}

void
_tm_begin(int value)
{
    /* Nothing to do */
}

static void
release_resources(struct resource* beg,
            const struct resource* end, bool commit)
{
    while (beg < end) {
        release_resource(beg, commit);
        ++beg;
    }
}

void
_tm_commit()
{
    release_resources(arraybeg(g_resource),
                      arrayend(g_resource), true);
}

void
tm_restart()
{
    release_resources(arraybeg(g_resource),
                      arrayend(g_resource), false);

    /* Jump to the beginning of the transaction */
    longjmp(_tm_get_tx()->env, 1);
}
