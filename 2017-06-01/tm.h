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

#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
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

void
privatize(uintptr_t addr, size_t siz, bool load, bool store);

void
load(uintptr_t addr, void* buf, size_t siz);

void
store(uintptr_t addr, const void* buf, size_t siz);

int
load_int(const int* addr);

void
store_int(int* addr, int value);
