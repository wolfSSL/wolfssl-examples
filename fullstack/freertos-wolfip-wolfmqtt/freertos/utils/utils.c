#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct event_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int value;
} event_t;

event_t *event_create(void) {
    event_t *event = malloc(sizeof(event_t));
    if (event != NULL) {
        pthread_mutex_init(&event->mutex, NULL);
        pthread_cond_init(&event->cond, NULL);
        event->value = 0;
    }
    return event;
}

void event_delete(event_t *event) {
    if (event != NULL) {
        pthread_mutex_destroy(&event->mutex);
        pthread_cond_destroy(&event->cond);
        free(event);
    }
}

void event_signal(event_t *event) {
    if (event != NULL) {
        pthread_mutex_lock(&event->mutex);
        event->value = 1;
        pthread_cond_signal(&event->cond);
        pthread_mutex_unlock(&event->mutex);
    }
}

void event_wait(event_t *event) {
    if (event != NULL) {
        pthread_mutex_lock(&event->mutex);
        while (event->value == 0) {
            pthread_cond_wait(&event->cond, &event->mutex);
        }
        event->value = 0;
        pthread_mutex_unlock(&event->mutex);
    }
}
