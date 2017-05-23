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
#include <stdint.h>

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
 * Resources
 */

#define RESOURCE_BITSHIFT   (3)
#define RESOURCE_NBYTES     (1ul << RESOURCE_BITSHIFT)
#define RESOURCE_BITMASK    ((1ul << RESOURCE_BITSHIFT) - 1)

/**
 * An integer value with an associated owner.
 */
struct resource {
    uintptr_t       base;
    uint8_t         local_value[RESOURCE_NBYTES];
    uint16_t        local_bits;
    pthread_t       owner;
    pthread_mutex_t lock;
};

#define RESOURCE_INITIALIZER \
    { \
        .addr = 0, \
        .local_bits = 0, \
        .owner = 0, \
        .lock  = PTHREAD_MUTEX_INITIALIZER \
    }

/* Resource helpers
 */

struct resource*
acquire_resource(uintptr_t base);

void
release_resource(struct resource* res, bool commit);

void
load(uintptr_t addr, void* buf, size_t siz);

void
store(uintptr_t addr, const void* buf, size_t siz);

int
load_int(const int* addr);

void
store_int(int* addr, int value);
