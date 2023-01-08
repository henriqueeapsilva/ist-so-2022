#include "fs/operations.h"
#include "utils/thread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILES (16)

char const content[] = "BBB! BBB!";

void *write(void *);

int main() {
    char filename[FILES][MAX_FILE_NAME];
    pthread_t thread[FILES];

    assert(tfs_init(NULL) != -1);

    for (int i = 0; i < FILES; i++) {
        snprintf(filename[i], MAX_FILE_NAME, "/f%d", i);
    }

    for (int i = 0; i < FILES; i++) {
        /*
         * Each thread will create a file and write to it,
         * then read and check if the content was written as intended.
         */
        thread_create(&thread[i], write, *(filename + i));
    }

    for (int i = 0; i < FILES; i++) {
        thread_join(&thread[i], NULL);
    }

    assert(tfs_destroy() != -1);
    puts("Successful test.");
    return EXIT_SUCCESS;
}

void *write(void *arg) {
    char *filename = (char *)arg;

    size_t len = strlen(content);
    char buffer[len];

    int f = tfs_open(filename, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_write(f, content, len) == len);
    assert(tfs_read(f, buffer, len) == len);
    assert(!memcmp(buffer, content, len));
    assert(tfs_close(f) != -1);

    return NULL;
}
