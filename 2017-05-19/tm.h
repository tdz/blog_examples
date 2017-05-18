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
#include <stdbool.h>

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

bool
acquire_int_resource(struct int_resource* res);

void
release_int_resource(struct int_resource* res, bool commit);

bool
load_int(struct int_resource* res, int* value);

bool
store_int(struct int_resource* res, int value);
