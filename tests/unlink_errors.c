#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "AAA!";
char const target_path[] = "/f1";
char const link_path[] = "/l1";
char const link_path2[] = "/l2";

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

int main() {
    assert(tfs_init(NULL) != -1);

    {
        /*
        Scenario 1: Unlinking open files should be postponed to when they are closed.

        It is not an error though. The file should be removed from its directory and the
        links count should be decremented, but the inode is intact so that it can be used
        by IO operations until no more instances of open files remain.
        */
        uint8_t buffer[sizeof(file_contents)];
        int f1;
        int f2;
        ssize_t w, r;

        f1 = tfs_open(target_path, TFS_O_CREAT);
        f2 = tfs_open(target_path, TFS_O_APPEND);
        assert(f1 != -1);
        assert(f2 != -1);

        assert_empty_file(target_path); // sanity check

        assert(tfs_unlink(target_path) != -1); // remove from directory but don't delete inode yet
        assert(tfs_open(target_path, TFS_O_TRUNC | TFS_O_APPEND) == -1); // we should not be able to open it anymore

        w = tfs_write(f1, file_contents, sizeof(file_contents)); // we can still write content though
        assert(w == sizeof(file_contents));

        r = tfs_read(f2, buffer, sizeof(buffer)); // assert content was written
        assert(r == sizeof(buffer));
        assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

        assert(tfs_close(f1) != -1); // now we can close the files
        assert(tfs_close(f2) != -1);
    }

    {
        int f = tfs_open(target_path, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);

        write_contents(target_path);

        assert_contents_ok(target_path); // sanity check
    }

    /*
    Scenario 2: Unlinked files should not exist anymore.
    */
    assert(tfs_link(target_path, link_path) != -1);
    assert(tfs_link(link_path, link_path2) != -1);
    assert(tfs_unlink(link_path2) != -1);
    assert(tfs_unlink(link_path) != -1);
    assert(tfs_unlink(target_path) != -1);
    assert(tfs_open(target_path, TFS_O_TRUNC | TFS_O_APPEND) == -1);
    assert(tfs_open(link_path, TFS_O_TRUNC | TFS_O_APPEND) == -1);
    assert(tfs_open(link_path2, TFS_O_TRUNC | TFS_O_APPEND) == -1);

    /*
    Scenario 3: Unlinking an unknown file.
    */
    assert(tfs_unlink(target_path) == -1);

    printf("Successful test.\n");

    return 0;
}
