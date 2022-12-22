#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

const char content[] = "TEXT";
const char filename[] = "/f1";
const char link_name_1[] = "/link_path1";
const char link_name_2[] = "/link_path2";

void *create_hardlink(void *arg);
void *create_symbolic_link(void *arg);

int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t tid[2];

    int f = tfs_open(filename, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);

    assert(pthread_create(&tid[0], NULL, create_hardlink, NULL) == 0);

    assert(pthread_create(&tid[1], NULL, create_symbolic_link, NULL) == 0);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}

void *create_hardlink(void *arg){
    (void)arg;
    assert(tfs_link(filename, link_name_1) == 0);
    {
        int f = tfs_open(link_name_1, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_write(f,content, sizeof(content)) == strlen(content));
        assert(tfs_close(f) != -1);
    }
    return NULL;
}

void *create_symbolic_link(void *arg){
    (void)arg;
    char buffer[200];

    assert(tfs_sym_link(filename, link_name_2) == 0);
    {
        int f = tfs_open(link_name_2, TFS_O_APPEND);
        assert(f != -1);
        assert(tfs_read(f,buffer, sizeof(content)) == sizeof(content));
        assert((tfs_close(f) != -1));
    }

    return NULL;
}