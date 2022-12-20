#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define NUM_THREADS (50)

char const filename[] = "/f1";

void *write_contents(void *arg);
void print_content(int thread_num);

int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t tid[NUM_THREADS];

    {
        int f = tfs_open(filename, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&tid[i], NULL, write_contents, NULL);
        // print_content(i);
    }

    // print_content(NUM_THREADS);
    // fflush(stdout);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    int f = tfs_open(filename, TFS_O_CREAT);
    assert (f != -1);

    // read result
    char buffer[NUM_THREADS];
    ssize_t r = tfs_read(f, buffer, NUM_THREADS);

    printf("%lu\n", r);
    fflush(stdout);

    // check result
    assert(r == NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++) {
       assert(buffer[i] == 'A');
    }

    assert(tfs_close(f) != -1);
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}

void *write_contents(void *arg) {
    (void)arg;

    int f = tfs_open(filename, TFS_O_APPEND);
    assert(f != -1);

    char content[] = "A";

    assert(tfs_write(f, content, 1) == 1);

    assert(tfs_close(f) != -1);

    return NULL;
}

void print_content(int thread_num) {
    int f = tfs_open(filename, TFS_O_CREAT);
    assert(f != -1);

    char buffer[NUM_THREADS+1];
    tfs_read(f, buffer, NUM_THREADS);
    buffer[NUM_THREADS] = '\0';
    printf("Thread #%d: %s\n", thread_num, buffer);

    assert(tfs_close(f) != -1);
}