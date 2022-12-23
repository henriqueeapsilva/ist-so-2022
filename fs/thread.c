#include "thread.h"
#include <stdio.h>
#include <stdlib.h>

mutex_t *mutex_alloc() {
  mutex_t *mutex = malloc(sizeof(mutex_t));

  if (mutex) {
    mutex_init(mutex);
  }

  return mutex;
}

void mutex_init(mutex_t *mutex) {
  if (pthread_mutex_init(mutex, NULL) == 0) {
    fprintf(stderr, "Could not initialize mutex.");
    exit(EXIT_FAILURE);
  }
}

void mutex_lock(mutex_t *mutex) {
  if (pthread_mutex_lock(mutex) != 0) {
    fprintf(stderr, "Could not lock mutex.");
    exit(EXIT_FAILURE);
  }
}

void mutex_unlock(mutex_t *mutex) {
  if (pthread_mutex_unlock(mutex) != 0) {
    fprintf(stderr, "Could not unlock mutex.");
    exit(EXIT_FAILURE);
  }
}

void mutex_destroy(mutex_t *mutex) {
  if (pthread_mutex_destroy(mutex) != 0) {
    fprintf(stderr, "Could not destroy mutex.");
    exit(EXIT_FAILURE);
  }
}

void mutex_free(mutex_t *mutex) {
  mutex_destroy(mutex);
  free(mutex);
}

rwlock_t *rwlock_alloc() {
  rwlock_t *lock = malloc(sizeof(rwlock_t));

  if (lock) {
    rwlock_init(lock);
  }

  return lock;
}

void rwlock_init(rwlock_t *rwlock) {
  if (pthread_rwlock_init(rwlock, NULL) != 0) {
    fprintf(stderr, "Could not initialize rwlock.");
    exit(EXIT_FAILURE);
  }
}

void rwlock_rdlock(rwlock_t *rwlock) {
  if (pthread_rwlock_rdlock(rwlock) != 0) {
    fprintf(stderr, "Could not read-lock rwlock.");
    exit(EXIT_FAILURE);
  }
}

void rwlock_wrlock(rwlock_t *rwlock) {
  if (pthread_rwlock_wrlock(rwlock) != 0) {
    fprintf(stderr, "Could not write-lock rwlock.");
    exit(EXIT_FAILURE);
  }
}

void rwlock_unlock(rwlock_t *rwlock) {
  if (pthread_rwlock_unlock(rwlock) != 0) {
    fprintf(stderr, "Could not unlock rwlock.");
    exit(EXIT_FAILURE);
  }
}

void rwlock_destroy(rwlock_t *rwlock) {
  if (pthread_rwlock_destroy(rwlock) != 0) {
    fprintf(stderr, "Could not destroy rwlock.");
    exit(EXIT_FAILURE);
  }
}

void rwlock_free(rwlock_t *rwlock) {
  rwlock_destroy(rwlock);
  free(rwlock);
}
