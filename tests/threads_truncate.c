#include "fs/operations.h"
#include "fs/thread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS (16)

char filename[] = "/f1";
char const content[] = "A";

void *truncate(void*);

int main() {
    pthread_t thread[THREADS];

    assert(tfs_init(NULL) != -1);

    {
        int f = tfs_open(filename, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);
    }

    for (int i = 0; i < THREADS; i++) {
        thread_create(&thread[i], truncate, NULL);
    }

    for (int i = 0; i < THREADS; i++) {
        thread_join(&thread[i], NULL);
    }
    
    {
        char buffer[5];
        int f = tfs_open(filename, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_read(f, buffer, 5) == 1);
        assert(buffer[0] == 'A');
        assert(tfs_close(f) != -1);
    }

    assert(tfs_destroy() != -1);
    puts("Successful test.");
    return EXIT_SUCCESS;
}

void *truncate(void* arg) {
    (void)arg;

    int f = tfs_open(filename, TFS_O_TRUNC);
    assert(f != -1);
    assert(tfs_write(f, content, 1) == 1);
    assert(tfs_close(f) != -1);

    return NULL;
}
