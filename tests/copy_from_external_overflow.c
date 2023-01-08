#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    char *str_ext_file =
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! ";
    char *tfs_path = "/f1";
    char *ext_path = "tests/file_large.txt";
    char buffer[600];

    tfs_params params = tfs_default_params();
    params.block_size = 256;

    assert(tfs_init(&params) != -1);

    int f;
    ssize_t r;

    /*
    Block size not big enough to receive all data from external file.
    This is not an error though.
    Data should be copied until no more space is available in the block.
    */

    f = tfs_copy_from_external_fs(ext_path, tfs_path);
    assert(f != -1);

    f = tfs_open(tfs_path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == params.block_size);
    assert(!memcmp(buffer, str_ext_file, params.block_size));

    printf("Successful test.\n");

    return 0;
}
