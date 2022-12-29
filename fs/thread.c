#include "thread.h"
#include <stdio.h>
#include <stdlib.h>

void thread_create(thread_t *thread, void *(*func)(void*), void *arg) {
  if (pthread_create(thread, NULL, func, arg)) {
    fprintf(stderr, "Could not create thread.\n");
    exit(EXIT_FAILURE);
  }
}

void thread_join(thread_t *thread, void **result) {
  if (pthread_join(*thread, result)) {
    fprintf(stderr, "Could not create thread.\n");
    exit(EXIT_FAILURE);
  }
}

void mutex_init(mutex_t *mutex) {
  if (pthread_mutex_init(mutex, NULL)) {
    fprintf(stderr, "Could not initialize mutex.\n");
    exit(EXIT_FAILURE);
  }
}

void mutex_lock(mutex_t *mutex) {
  if (pthread_mutex_lock(mutex)) {
    fprintf(stderr, "Could not lock mutex.\n");
    exit(EXIT_FAILURE);
  }
}

void mutex_unlock(mutex_t *mutex) {
  if (pthread_mutex_unlock(mutex)) {
    fprintf(stderr, "Could not unlock mutex.\n");
    exit(EXIT_FAILURE);
  }
}

void mutex_destroy(mutex_t *mutex) {
  if (pthread_mutex_destroy(mutex)) {
    fprintf(stderr, "Could not destroy mutex.\n");
    exit(EXIT_FAILURE);
  }
}

void rwlock_init(rwlock_t *rwlock) {
  if (pthread_rwlock_init(rwlock, NULL)) {
    fprintf(stderr, "Could not initialize rwlock.\n");
    exit(EXIT_FAILURE);
  }
}

void rwlock_rdlock(rwlock_t *rwlock) {
  if (pthread_rwlock_rdlock(rwlock)) {
    fprintf(stderr, "Could not read-lock rwlock.\n");
    exit(EXIT_FAILURE);
  }
}

void rwlock_wrlock(rwlock_t *rwlock) {
  if (pthread_rwlock_wrlock(rwlock)) {
    fprintf(stderr, "Could not write-lock rwlock.\n");
    exit(EXIT_FAILURE);
  }
}

void rwlock_unlock(rwlock_t *rwlock) {
  if (pthread_rwlock_unlock(rwlock)) {
    fprintf(stderr, "Could not unlock rwlock.\n");
    exit(EXIT_FAILURE);
  }
}

void rwlock_destroy(rwlock_t *rwlock) {
  if (pthread_rwlock_destroy(rwlock)) {
    fprintf(stderr, "Could not destroy rwlock.\n");
    exit(EXIT_FAILURE);
  }
}
