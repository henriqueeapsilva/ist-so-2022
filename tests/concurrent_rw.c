#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define NUM_WRITERS (15)
#define NUM_READERS (15)

const char content[] = "ABBA";
const char filename[] = "/f1";

void *read_content(void *arg);
void *write_content(void *arg);

int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t writers[NUM_WRITERS];
    pthread_t readers[NUM_READERS];

    /*
    Test with multiple readers and multiple writers.
    Readers only stop reading when all chars are written.
    */

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_create(&readers[i], NULL, read_content, NULL);
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_create(&writers[i], NULL, write_content, NULL);
    }

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}

void *read_content(void *arg) {
    (void)arg;

    while (1) {
        int f = tfs_open(filename, TFS_O_CREAT);
        assert(f != -1);

        size_t len = NUM_WRITERS * sizeof(content);
        char buffer[len];

        if (tfs_read(f, buffer, len) == len) break;

        assert(tfs_close(f) != -1);
    }

    return NULL;
}

void *write_content(void *arg) {
    (void)arg;

    int f = tfs_open(filename, TFS_O_APPEND);
    assert(f != -1);

    assert(tfs_write(f, content, sizeof(content)));

    assert(tfs_close(f) != -1);

    return NULL;
}