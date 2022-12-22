#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "AAA! BBB! CCC! That was easy!!!";
char const target_path1[] = "/f1";
char const link_path1[] = "/l1";
char const link_path2[] = "/l2";
char const link_path3[] = "/l3";
char const link_path4[] = "/l4";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

int main(){
    assert(tfs_init(NULL) != -1);

    {
        int f = tfs_open(target_path1, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);
    }
    // creates two hard links to the target_path1
    assert(tfs_link(target_path1, link_path1) != -1);
    assert(tfs_link(link_path1, link_path2) != -1);

    assert_empty_file(target_path1);

    // checks if of is written on the hardlinks for the same file is the same
    write_contents(link_path2);
    assert_contents_ok(link_path1);

    // creates the symbolic links on chain
    assert(tfs_sym_link(link_path2, link_path3) != -1);
    assert(tfs_sym_link(link_path3, link_path4) != -1);

    // unlinks the hardlink with no symbolic links
    assert(tfs_unlink(link_path1) != -1 );

    // checks if the files still exists with the symbolic links
    {
        int f = tfs_open(link_path4, TFS_O_APPEND);
        assert(f != -1);
        assert(tfs_close(f) != -1);
    }

    // check if the contents are the same as before and if nothing was lost
    assert_contents_ok(link_path3);

    printf("Successful test.\n");

}