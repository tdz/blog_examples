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
#include <stdint.h>

#define BASE_BITMASK        (~RESOURCE_BITMASK)

#define RESOURCE_BITSHIFT   (3)
#define RESOURCE_NBYTES     (1ul << RESOURCE_BITSHIFT)
#define RESOURCE_BITMASK    ((1ul << RESOURCE_BITSHIFT) - 1)

/**
 * A value with an associated owner.
 */
struct resource {
    uintptr_t       base;
    uint8_t         local_value[RESOURCE_NBYTES];
    uint16_t        local_bits;
    pthread_t       owner;
    pthread_mutex_t lock;
};

#define NRESOURCES_BITSHIFT (10)
#define NRESOURCES          (1ul << NRESOURCES_BITSHIFT)
#define NRESOURCES_BITMASK  ((1ul << NRESOURCES_BITSHIFT) - 1)

extern struct resource g_resource[NRESOURCES];

struct resource*
acquire_resource(uintptr_t base);

void
release_resource(struct resource* res, bool commit);
