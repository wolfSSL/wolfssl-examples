/* posix_utils.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>

/* Event handling for POSIX port */
typedef struct event_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int signaled;
} event_t;

/* Create event */
void* event_create(void)
{
    event_t* event = (event_t*)malloc(sizeof(event_t));
    if (event) {
        pthread_mutex_init(&event->mutex, NULL);
        pthread_cond_init(&event->cond, NULL);
        event->signaled = 0;
    }
    return event;
}

/* Wait for event */
int event_wait(void* handle)
{
    event_t* event = (event_t*)handle;
    int ret = 0;

    if (!event) {
        return -1;
    }

    pthread_mutex_lock(&event->mutex);
    while (!event->signaled) {
        ret = pthread_cond_wait(&event->cond, &event->mutex);
        if (ret != 0) {
            break;
        }
    }
    event->signaled = 0;
    pthread_mutex_unlock(&event->mutex);

    return ret;
}

/* Signal event */
int event_signal(void* handle)
{
    event_t* event = (event_t*)handle;
    int ret = 0;

    if (!event) {
        return -1;
    }

    pthread_mutex_lock(&event->mutex);
    event->signaled = 1;
    ret = pthread_cond_signal(&event->cond);
    pthread_mutex_unlock(&event->mutex);

    return ret;
}

/* Delete event */
int event_delete(void* handle)
{
    event_t* event = (event_t*)handle;
    int ret = 0;

    if (!event) {
        return -1;
    }

    pthread_mutex_destroy(&event->mutex);
    pthread_cond_destroy(&event->cond);
    free(event);

    return ret;
}
