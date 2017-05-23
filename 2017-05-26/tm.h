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

#pragma once

#include <pthread.h>
#include <setjmp.h>
#include <stdbool.h>

/*
 * Transaction beginning and end
 */

#define tm_save volatile

struct _tm_tx {
    jmp_buf env;
};

struct _tm_tx*
_tm_get_tx(void);

void
_tm_begin(int value);

void
_tm_commit(void);

#define tm_begin                            \
    _tm_begin(setjmp(_tm_get_tx()->env));   \
    {

#define tm_commit                           \
        _tm_commit();                       \
    }

void
tm_restart(void);

/*
 * Integer resources
 */

#define RESOURCE_HAS_LOCAL_VALUE    (1ul << 0)

/**
 * An integer value with an associated owner.
 */
struct int_resource {
    int             value;
    int             local_value;
    unsigned long   flags;
    pthread_t       owner;
    pthread_mutex_t lock;
};

#define INT_RESOURCE_INITIALIZER \
    { \
        .value = 0, \
        .local_value = 0, \
        .flags = 0, \
        .owner = 0, \
        .lock  = PTHREAD_MUTEX_INITIALIZER \
    }

/**
 * The globally shared resources.
 */
struct int_resource g_int_resource[2];

/* Resource helpers
 */

bool
acquire_int_resource(struct int_resource* res);

void
release_int_resource(struct int_resource* res, bool commit);

void
load_int(struct int_resource* res, int* value);

void
store_int(struct int_resource* res, int value);
