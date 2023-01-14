#include "thread.h"
#include <stdio.h>
#include <stdlib.h>

static void quit(char *reason) {
    fprintf(stderr, "%s\n", reason);
    exit(EXIT_FAILURE);
}

void thread_create(thread_t *thread, void *(*func)(void *), void *arg) {
    if (pthread_create(thread, NULL, func, arg)) {
        quit("Could not create thread.");
    }
}

void thread_join(thread_t *thread, void **result) {
    if (pthread_join(*thread, result)) {
        quit("Could not create thread.");
    }
}

void mutex_init(mutex_t *mutex) {
    if (pthread_mutex_init(mutex, NULL)) {
        quit("Could not initialize mutex.");
    }
}

void mutex_lock(mutex_t *mutex) {
    if (pthread_mutex_lock(mutex)) {
        quit("Could not lock mutex.");
    }
}

void mutex_unlock(mutex_t *mutex) {
    if (pthread_mutex_unlock(mutex)) {
        quit("Could not unlock mutex.");
    }
}

void mutex_destroy(mutex_t *mutex) {
    if (pthread_mutex_destroy(mutex)) {
        quit("Could not destroy mutex.");
    }
}

void rwlock_init(rwlock_t *rwlock) {
    if (pthread_rwlock_init(rwlock, NULL)) {
        quit("Could not initialize rwlock.");
    }
}

void rwlock_rdlock(rwlock_t *rwlock) {
    if (pthread_rwlock_rdlock(rwlock)) {
        quit("Could not read-lock rwlock.");
    }
}

void rwlock_wrlock(rwlock_t *rwlock) {
    if (pthread_rwlock_wrlock(rwlock)) {
        quit("Could not write-lock rwlock.");
    }
}

void rwlock_unlock(rwlock_t *rwlock) {
    if (pthread_rwlock_unlock(rwlock)) {
        quit("Could not unlock rwlock.");
    }
}

void rwlock_destroy(rwlock_t *rwlock) {
    if (pthread_rwlock_destroy(rwlock)) {
        quit("Could not destroy rwlock.");
    }
}

void cond_init(cond_t *cond) {
    if(pthread_cond_init(cond, NULL)) {
        quit("Could not init cond var.");
    }
}

void cond_destroy(cond_t *cond) {
    if(pthread_cond_destroy(cond)) {
        quit("Could not destroy cond var.");
    }
}

void cond_wait(cond_t *cond, mutex_t *mutex) {
    if(pthread_cond_wait(cond, mutex)) {
        quit("Could not wait - cond var.");
    }
}

void cond_signal(cond_t *cond) {
    if(pthread_cond_signal(cond)) {
        quit("Could not send signal from cond var.");
    }
}