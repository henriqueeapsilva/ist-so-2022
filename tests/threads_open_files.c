#include "fs/operations.h"
#include "utils/thread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILES (20)

char const content[] = "A";

void *openclose(void*);
void assert_content(char*);

int main() {
    pthread_t thread[FILES];
    char filename[FILES][MAX_FILE_NAME];

    assert(tfs_init(NULL) != -1);

    for (int i = 0; i < FILES; i++) {
        snprintf(filename[i], 6, "/f%d", i);
    }

    for (int i = 0; i < FILES; i++) {
        /*
         * Each thread will try to:
         * 1. Open (and create) a different file.
         * 2. Write a predefined content to it.
         * 3. Close the file.
         * 
         * This is a very simple test. 
         */
        thread_create(&thread[i], openclose, *(filename+i));
    }

    for (int i = 0; i < FILES; i++) {
        thread_join(&thread[i], NULL);
    }

    for (int i = 0; i < FILES; i++) {
        assert_content(filename[i]);
    }

    assert(tfs_destroy() != -1);

    puts("Successful test.");

    return EXIT_SUCCESS;
}

void *openclose(void* arg) {
    char *filename = (char*) arg;

    int f = tfs_open(filename, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, content, strlen(content)) == strlen(content));
    assert(tfs_close(f) != -1);

    return NULL;
}

void assert_content(char *filename) {
    char buffer[sizeof(content)+5];

    int f = tfs_open(filename, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_read(f, buffer, sizeof(content)+5) == strlen(content));
    assert(tfs_close(f) != -1);
}