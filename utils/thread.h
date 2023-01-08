#ifndef _LOCK_H_
#define _LOCK_H_

#include <pthread.h>

typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_rwlock_t rwlock_t;

void thread_create(thread_t *thread, void *(*func)(void *), void *arg);
void thread_join(thread_t *thread, void **result);

void mutex_init(mutex_t *);
void mutex_lock(mutex_t *);
void mutex_unlock(mutex_t *);
void mutex_destroy(mutex_t *);

void rwlock_init(rwlock_t *);
void rwlock_rdlock(rwlock_t *);
void rwlock_wrlock(rwlock_t *);
void rwlock_unlock(rwlock_t *);
void rwlock_destroy(rwlock_t *);

#endif