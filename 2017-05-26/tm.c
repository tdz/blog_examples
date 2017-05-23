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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define arraylen(_array)    \
    ( sizeof(_array) / sizeof(*(_array)) )

#define arraybeg(_array)    \
    ( _array )

#define arrayend(_array)    \
    ( arraybeg(_array) + arraylen(_array) )

#define BASE_BITMASK        (~RESOURCE_BITMASK)

#define NRESOURCES_BITSHIFT (10)
#define NRESOURCES          (1ul << NRESOURCES_BITSHIFT)
#define NRESOURCES_MASK     ((1ul << NRESOURCES_BITSHIFT) - 1)

struct resource g_resource[NRESOURCES];

static struct resource*
find_resource(uintptr_t base)
{
    unsigned long element = (base >> RESOURCE_BITSHIFT) & NRESOURCES_MASK;

    return g_resource + element;
}

struct resource*
acquire_resource(uintptr_t base)
{
    struct resource* res = find_resource(base);

    pthread_t self = pthread_self();

    int err = pthread_mutex_lock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_lock");
        goto err_pthread_mutex_lock;
    }

    if (res->owner && res->owner != self) {
        /* Owned by another thread. */
        goto err_has_owner;

    } else if (res->owner && res->owner == self) {
        /* Owned by us. */
        if (base != res->base) {
            abort(); /* We cannot re-use the resource with a different base. */
        }

    } else if (!res->owner) {
        /* Now owned by us. */
        res->base = base;
        res->owner = self;
    }

    err = pthread_mutex_unlock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_unlock");
        abort(); /* We cannot release; let's abort for now. */
    }

    return res;

err_has_owner:
    /* fall through */
err_pthread_mutex_lock:
    err = pthread_mutex_unlock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_unlock");
        abort(); /* We cannot release; let's abort for now. */
    }
    return NULL;
}

void
release_resource(struct resource* res, bool commit)
{
    pthread_t self = pthread_self();

    int err = pthread_mutex_lock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_lock");
        abort(); /* We cannot release; let's abort for now. */
    }

    if (res->owner && res->owner == self) {

        if (res->local_bits) {

            if (commit) {
                unsigned long bit = 1ul;

                uint8_t* mem = (uint8_t*)res->base;
                uint8_t* beg = arraybeg(res->local_value);
                uint8_t* end = arrayend(res->local_value);

                while (beg < end) {
                    if (res->local_bits & bit) {
                        *mem = *beg;
                    }
                    bit <<= 1;
                    ++mem;
                    ++beg;
                }
            }

            res->local_bits = 0;
        }

        res->owner = 0;
    }

    err = pthread_mutex_unlock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_unlock");
        abort(); /* We cannot release; let's abort for now. */
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
