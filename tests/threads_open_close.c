#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILES (20)

char filename[FILES][6];
char const content[] = "A";

void *openclose(void*);
void assert_content(int);

int main() {
    pthread_t thread[FILES];
    int index[FILES];

    assert(tfs_init(NULL) != -1);

    for (int i = 0; i < FILES; i++) {
        index[i] = i;
        snprintf(filename[i], 6, "/f%d", i);
        filename[i][5] = '\0';
    }

    for (int i = 0; i < FILES; i++) {
        assert(!pthread_create(&thread[i], NULL, (void*) (openclose), (index+i)));
    }

    for (int i = 0; i < FILES; i++) {
        assert(!pthread_join(thread[i], NULL));
    }

    for (int i = 0; i < FILES; i++) {
        assert_content(i);
    }

    assert(tfs_destroy() != -1);

    puts("Successful test.");

    return EXIT_SUCCESS;
}

void *openclose(void* arg) {
    int i = *((int*) arg);

    int f = tfs_open(filename[i], TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, content, 1) == 1);

    assert(tfs_close(f) != -1);

    return NULL;
}

void assert_content(int i) {
    char buffer[5];

    int f = tfs_open(filename[i], TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_read(f, buffer, 5) == 1);
    assert(tfs_close(f) != -1);
}