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

#include "res.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"

struct resource g_resource[NRESOURCES];

static struct resource*
find_resource(uintptr_t base)
{
    unsigned long element = (base >> RESOURCE_BITSHIFT) & NRESOURCES_BITMASK;

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

            /* We have to store if we either commit in write-back
             * mode, or revert in write-through mode.
             */
            bool store_local_bits = commit != !!(res->flags & RESOURCE_FLAG_WRITE_THROUGH);

            if (store_local_bits) {
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
            res->flags = 0;
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


