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

#define arraylen(_array)    \
    ( sizeof(_array) / sizeof(*(_array)) )

#define arraybeg(_array)    \
    ( _array )

#define arrayend(_array)    \
    ( arraybeg(_array) + arraylen(_array) )
