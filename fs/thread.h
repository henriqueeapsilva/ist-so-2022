#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>

typedef pthread_mutex_t mutex_t;
typedef pthread_rwlock_t rwlock_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);
void mutex_destroy(mutex_t *mutex);

void rwlock_init(rwlock_t *rwlock);
void rwlock_rdlock(rwlock_t *rwlock);
void rwlock_wrlock(rwlock_t *rwlock);
void rwlock_unlock(rwlock_t *rwlock);
void rwlock_destroy(rwlock_t *rwlock);

#endif