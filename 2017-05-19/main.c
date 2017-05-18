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

/*
 *  On your Linux terminal, run
 *
 *      make all
 *      ./simpletm
 *
 *  to build and execute this example. Building requires gcc and the
 *  usual C development tools for Unix.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tm.h"

static void
producer_func(void)
{
    unsigned int seed = 1;

    int i0 = 0;

    while (true) {

        sleep(1);

        ++i0;
        int i1 = rand_r(&seed);

        printf("Storing i0=%d, i1=%d\n", i0, i1);

        bool commit = false;

        while (!commit) {

            bool succ = store_int(g_int_resource + 0, i0);
            if (!succ) {
                goto release;
            }
            succ = store_int(g_int_resource + 1, i1);
            if (!succ) {
                goto release;
            }

            commit = true;

        release:
            release_int_resource(g_int_resource + 0, commit);
            release_int_resource(g_int_resource + 1, commit);
        }
    }
}

static void
verify_load(int i0, int i1)
{
    unsigned int seed = 1;

    int i;
    int value = 0;

    for (i = 0; i < i0; ++i) {
        value = rand_r(&seed);
    }

    if (value != i1) {
        printf("Incorrect value pair (%d,%d), should be (%d,%d)\n",
               i0, i1, i, value);
    }
}

static void
consumer_func(void)
{
    while (true) {

        sleep(1);

        int i0, i1;

        bool commit = false;

        while (!commit) {

            bool succ = load_int(g_int_resource + 1, &i1);
            if (!succ) {
                goto release;
            }
            succ = load_int(g_int_resource + 0, &i0);
            if (!succ) {
                goto release;
            }

            verify_load(i0, i1);

            commit = true;

        release:
            release_int_resource(g_int_resource + 0, commit);
            release_int_resource(g_int_resource + 1, commit);
        }

        printf("Loaded i0=%d, i1=%d\n", i0, i1);
    }
}

static void*
producer_func_cb(void* arg)
{
    producer_func();
    return NULL;
}

static void*
consumer_func_cb(void* arg)
{
    consumer_func();
    return NULL;
}

int
main(int argc, char* argv[])
{
    pthread_t consumer;
    int err = pthread_create(&consumer, NULL, consumer_func_cb, NULL);
    if (err) {
        errno = err;
        perror("pthread_create");
        goto err_pthread_create_consumer;
    }

    pthread_t producer;
    err = pthread_create(&producer, NULL, producer_func_cb, NULL);
    if (err) {
        errno = err;
        perror("pthread_create");
        goto err_pthread_create_producer;
    }

    err = pthread_join(producer, NULL);
    if (err) {
        errno = err;
        perror("pthread_join");
    }
    err = pthread_join(consumer, NULL);
    if (err) {
        errno = err;
        perror("pthread_join");
    }

    return EXIT_SUCCESS;

err_pthread_create_producer:
    err = pthread_join(consumer, NULL);
    if (err) {
        errno = err;
        perror("pthread_join");
    }
err_pthread_create_consumer:
    return EXIT_FAILURE;
}
