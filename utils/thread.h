#ifndef _LOCK_H_
#define _LOCK_H_

#include <pthread.h>

typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_rwlock_t rwlock_t;

/**
 * Creates a thread.
 * 
 * Input:
 *   - thread: a pointer to a thread (may be null)
 *   - func: thread's start point
 *   - arg: func's argument
 * 
 * In case of error, the program will quit.
 */
void thread_create(thread_t *thread, void *(*func)(void *), void *arg);

/**
 * Blocks until the given thread finishes.
 * 
 * Input:
 *   - thread: a pointer to a thread
 *   - result: pointer to thread's return
 * 
 * In case of error, the program will quit.
 */
void thread_join(thread_t *thread, void **result);

/**
 * Initializes the given mutex (may be null).
 * 
 * In case of error, the program will quit.
 */
void mutex_init(mutex_t *);

/**
 * Locks the given mutex.
 * 
 * In case of error, the program will quit.
 */
void mutex_lock(mutex_t *);

/**
 * Unlocks the given mutex.
 * 
 * In case of error, the program will quit.
 */
void mutex_unlock(mutex_t *);

/**
 * Destroys the given mutex.
 * 
 * In case of error, the program will quit.
 */
void mutex_destroy(mutex_t *);

/**
 * Initializes the given rwlock (may be lock).
 * 
 * In case of error, the program will quit.
 */
void rwlock_init(rwlock_t *);

/**
 * Read-locks the given rwlock.
 * 
 * In case of error, the program will quit.
 */
void rwlock_rdlock(rwlock_t *);

/**
 * Write-locks the given rwlock.
 * 
 * In case of error, the program will quit.
 */
void rwlock_wrlock(rwlock_t *);

/**
 * Unlocks the given rwlock.
 * 
 * In case of error, the program will quit.
 */
void rwlock_unlock(rwlock_t *);

/**
 * Destroys the given rwlock.
 * 
 * In case of error, the program will quit.
 */
void rwlock_destroy(rwlock_t *);

#endif