#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    char *str_ext_file = "BBB!";
    char *tfs_path = "/f1";
    char *ext_path = "tests/file_small.txt";
    char buffer[40];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r;

    f = tfs_copy_from_external_fs(ext_path, tfs_path);
    assert(f != -1);

    f = tfs_open(tfs_path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    // Repeat the copy to the same file
    f = tfs_copy_from_external_fs(ext_path, tfs_path);
    assert(f != -1);

    f = tfs_open(tfs_path, TFS_O_CREAT);
    assert(f != -1);

    // Contents should be overwriten, not appended
    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    printf("Successful test.\n");

    return 0;
}
