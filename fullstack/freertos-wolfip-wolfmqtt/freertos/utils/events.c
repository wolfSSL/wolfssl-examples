/* events.c
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
#include <errno.h>
#include <time.h>
#include "events.h"

/* Event structure */
typedef struct event_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int signaled;
} event_t;

/* Create a new event */
void *event_create(void) {
    event_t *event = (event_t *)malloc(sizeof(event_t));
    if (event == NULL) {
        return NULL;
    }

    if (pthread_mutex_init(&event->mutex, NULL) != 0) {
        free(event);
        return NULL;
    }

    if (pthread_cond_init(&event->cond, NULL) != 0) {
        pthread_mutex_destroy(&event->mutex);
        free(event);
        return NULL;
    }

    event->signaled = 0;
    return event;
}

/* Delete an event */
void event_delete(void *event_handle) {
    event_t *event = (event_t *)event_handle;
    if (event != NULL) {
        pthread_mutex_destroy(&event->mutex);
        pthread_cond_destroy(&event->cond);
        free(event);
    }
}

/* Signal an event */
void event_signal(void *event_handle) {
    event_t *event = (event_t *)event_handle;
    if (event != NULL) {
        pthread_mutex_lock(&event->mutex);
        event->signaled = 1;
        pthread_cond_signal(&event->cond);
        pthread_mutex_unlock(&event->mutex);
    }
}

/* Wait for an event */
void event_wait(void *event_handle) {
    event_t *event = (event_t *)event_handle;
    if (event != NULL) {
        pthread_mutex_lock(&event->mutex);
        while (event->signaled == 0) {
            pthread_cond_wait(&event->cond, &event->mutex);
        }
        event->signaled = 0;
        pthread_mutex_unlock(&event->mutex);
    }
}

/* Wait for an event with timeout */
int event_wait_timeout(void *event_handle, unsigned int timeout_ms) {
    event_t *event = (event_t *)event_handle;
    struct timespec ts;
    int ret = 0;

    if (event == NULL) {
        return -1;
    }

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    pthread_mutex_lock(&event->mutex);
    while (event->signaled == 0) {
        ret = pthread_cond_timedwait(&event->cond, &event->mutex, &ts);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&event->mutex);
            return 1; /* Timeout */
        }
    }
    event->signaled = 0;
    pthread_mutex_unlock(&event->mutex);

    return 0; /* Success */
}
