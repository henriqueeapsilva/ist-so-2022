#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    char *str_ext_file =
            "simple test to cover all the implementation done AAA! BBB! CCC! DDD! EEE! FFF! GGG!!!!!!!!!!!!";
    char *path_copied_file = "/f1";
    char *path_src = "tests/file_to_copy_over_test.txt";
    char *path_error = "tests/unknown_file.txt";
    char buffer[600];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r;

    f = tfs_copy_from_external_fs(path_error, path_copied_file);
    assert(f == -1);

    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    printf("Successful test.\n");

    return 0;
}
