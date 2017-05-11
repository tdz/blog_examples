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
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * An integer value with an associated owner.
 */
struct int_resource {
    int             value;
    pthread_t       owner;
    pthread_mutex_t lock;
};

#define INT_RESOURCE_INITIALIZER \
    { \
        .value = 0, \
        .owner = 0, \
        .lock  = PTHREAD_MUTEX_INITIALIZER \
    }

static bool
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

static void
release_int_resource(struct int_resource* res)
{
    pthread_t self = pthread_self();

    int err = pthread_mutex_lock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_lock");
        abort(); /* We cannot release; let's abort for now. */
    }

    if (res->owner && res->owner == self) {
        res->owner = 0;
    }

    err = pthread_mutex_unlock(&res->lock);
    if (err) {
        errno = err;
        perror("pthread_mutex_unlock");
        abort(); /* We cannot release; let's abort for now. */
    }
}

static bool
load_int(struct int_resource* res, int* value)
{
    bool succ = acquire_int_resource(res);
    if (!succ) {
        return false;
    }

    *value = res->value;

    return true;
}

static bool
store_int(struct int_resource* res, int value)
{
    bool succ = acquire_int_resource(res);
    if (!succ) {
        return false;
    }

    res->value = value;

    return true;
}

/**
 * The globally shared resources.
 */
static struct int_resource g_int_resource[] = {
    INT_RESOURCE_INITIALIZER,
    INT_RESOURCE_INITIALIZER
};

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
            release_int_resource(g_int_resource + 0);
            release_int_resource(g_int_resource + 1);
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
            release_int_resource(g_int_resource + 0);
            release_int_resource(g_int_resource + 1);
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
