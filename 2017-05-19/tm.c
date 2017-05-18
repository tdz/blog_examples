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

struct int_resource g_int_resource[2] = {
    INT_RESOURCE_INITIALIZER,
    INT_RESOURCE_INITIALIZER
};

bool
acquire_int_resource(struct int_resource* res)
{
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

    } else if (!res->owner) {
        /* Now owned by us. */
        res->owner = self;
    }

    err = pthread_mutex_unlock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_unlock");
        abort(); /* We cannot release; let's abort for now. */
    }

    return true;

err_has_owner:
    /* fall through */
err_pthread_mutex_lock:
    err = pthread_mutex_unlock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_unlock");
        abort(); /* We cannot release; let's abort for now. */
    }
    return false;
}

void
release_int_resource(struct int_resource* res, bool commit)
{
    pthread_t self = pthread_self();

    int err = pthread_mutex_lock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_lock");
        abort(); /* We cannot release; let's abort for now. */
    }

    if (res->owner && res->owner == self) {

        if (res->flags & RESOURCE_HAS_LOCAL_VALUE) {
            if (commit) {
                res->value = res->local_value;
            }
            res->flags &= ~RESOURCE_HAS_LOCAL_VALUE;
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
load_int(struct int_resource* res, int* value)
{
    bool succ = acquire_int_resource(res);
    if (!succ) {
        tm_restart();
    }

    if (res->flags & RESOURCE_HAS_LOCAL_VALUE) {
        *value = res->local_value;
    } else {
        *value = res->value;
    }
}

void
store_int(struct int_resource* res, int value)
{
    bool succ = acquire_int_resource(res);
    if (!succ) {
        tm_restart();
    }

    res->local_value = value;
    res->flags |= RESOURCE_HAS_LOCAL_VALUE;
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

#define arraylen(_array)    \
    ( sizeof(_array) / sizeof(*(_array)) )

#define arraybeg(_array)    \
    ( _array )

#define arrayend(_array)    \
    ( arraybeg(_array) + arraylen(_array) )

static void
release_int_resources(struct int_resource* beg,
                const struct int_resource* end, bool commit)
{
    while (beg < end) {
        release_int_resource(beg, commit);
        ++beg;
    }
}

void
_tm_commit()
{
    release_int_resources(arraybeg(g_int_resource),
                          arrayend(g_int_resource), true);
}

void
tm_restart()
{
    release_int_resources(arraybeg(g_int_resource),
                          arrayend(g_int_resource), false);

    /* Jump to the beginning of the transaction */
    longjmp(_tm_get_tx()->env, 1);
}
