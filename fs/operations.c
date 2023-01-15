#include "operations.h"
#include "../utils/betterassert.h"
#include "../utils/logging.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params = params_ptr ? *params_ptr : tfs_default_params();

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    if (inode_create(T_DIRECTORY) != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() { return state_destroy(); }

static bool valid_pathname(char const *name) {
    return (name != NULL) && (strlen(name) > 1) && (name[0] == '/');
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    if (!valid_pathname(name)) {
        return -1;
    }

    wrlock_inode(ROOT_DIR_INUM);

    inode_t *root = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root, "tfs_open: root directory inode must exist");

    int inum = find_in_dir(root, name + 1);

    // if not found, create new file
    if (inum < 0) {
        if (!(mode & TFS_O_CREAT)) {
            unlock_inode(ROOT_DIR_INUM);
            return -1;
        }

        inum = inode_create(T_FILE);

        if (inum < 0) {
            DEBUG("inode_create failed.");
            unlock_inode(ROOT_DIR_INUM);
            return -1;
        }

        if (add_dir_entry(root, name + 1, inum)) {
            DEBUG("Failed adding to directory.");
            inode_delete(inum);
            unlock_inode(ROOT_DIR_INUM);
            return -1;
        }
    }

    wrlock_inode(inum);
    unlock_inode(ROOT_DIR_INUM);

    inode_t *inode = inode_get(inum);

    ALWAYS_ASSERT(inode, "tfs_open: inode deleted mid operation");

    if (inode->i_type == T_LINK) {
        name = data_block_get(inode->i_bnumber);
        ALWAYS_ASSERT(name, "tfs_open: empty symlink");

        unlock_inode(inum);
        return tfs_open(name, mode);
    }

    // Truncate
    if ((mode & TFS_O_TRUNC) && (inode->i_size > 0)) {
        inode->i_size = 0;
        data_block_free(inode->i_bnumber);
    }

    size_t offset = (mode & TFS_O_APPEND) * inode->i_size;

    unlock_inode(inum);
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (!file) {
        return -1;
    }

    remove_from_open_file_table(fhandle);

    int inum = file->of_inumber;

    wrlock_inode(inum);

    inode_t *inode = inode_get(inum);
    if (!inode || (inode->i_links > 0)) {
        unlock_inode(inum);
        return 0;
    }

    if (find_open_file_entry(inum) < 0) {
        inode_delete(inum);
    }

    unlock_inode(inum);
    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (!file) {
        return -1;
    }

    int inumber = file->of_inumber;

    wrlock_inode(inumber);

    inode_t *inode = inode_get(inumber);

    ALWAYS_ASSERT(inode, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();

    if ((to_write + file->of_offset) > block_size) {
        to_write = (block_size - file->of_offset);
    }

    if (to_write == 0) {
        unlock_inode(inumber);
        return 0;
    }

    if (inode->i_size == 0) {
        inode->i_bnumber = data_block_alloc();

        if (inode->i_bnumber == -1) {
            unlock_inode(inumber);
            return -1; // no space
        }
    }

    void *block = data_block_get(inode->i_bnumber);

    ALWAYS_ASSERT(block, "tfs_write: data block deleted mid-write");

    memcpy((block + file->of_offset), buffer, to_write);

    if ((file->of_offset += to_write) > inode->i_size) {
        inode->i_size = file->of_offset;
    }

    unlock_inode(inumber);
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    int inumber = file->of_inumber;

    rdlock_inode(inumber);

    inode_t const *inode = inode_get(inumber);

    ALWAYS_ASSERT(inode, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;

    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_bnumber);

        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, (block + file->of_offset), to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }

    unlock_inode(inumber);
    return (ssize_t)to_read;
}

int tfs_sym_link(char const *target, char const *link_name) {
    if (!valid_pathname(target)) {
        return -1;
    }

    if (!strcmp(target, link_name)) {
        return -1;
    }

    wrlock_inode(ROOT_DIR_INUM);

    inode_t *root = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root, "tfs_sym_link: root directory inode must exist");

    if (find_in_dir(root, link_name + 1) >= 0) {
        unlock_inode(ROOT_DIR_INUM);
        return -1; // file already exists
    }

    int inum = inode_create(T_LINK);
    if (inum < 0) {
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    inode_t *inode = inode_get(inum);
    ALWAYS_ASSERT(inode, "tfs_sym_link: inode deleted mid operation");

    inode->i_bnumber = data_block_alloc();
    if (inode->i_bnumber < 0) {
        inode_delete(inum);
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    if (add_dir_entry(root, link_name + 1, inum)) {
        inode_delete(inum);
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    char *block = data_block_get(inode->i_bnumber);
    ALWAYS_ASSERT(block, "tfs_sym_link: block deleted mid operation");

    strncpy(block, target, MAX_FILE_NAME - 1);
    block[MAX_FILE_NAME - 1] = '\0';

    unlock_inode(ROOT_DIR_INUM);
    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    wrlock_inode(ROOT_DIR_INUM);

    inode_t *root = inode_get(ROOT_DIR_INUM);

    int inum = find_in_dir(root, target + 1);
    if (inum == -1) {
        return -1;
    }

    wrlock_inode(inum);

    inode_t *inode = inode_get(inum);
    if (!inode || (inode->i_type == T_LINK)) {
        unlock_inode(inum);
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    if (add_dir_entry(root, link_name + 1, inum) == -1) {
        unlock_inode(inum);
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    unlock_inode(ROOT_DIR_INUM);

    inode->i_links++;

    unlock_inode(inum);
    return 0;
}

int tfs_unlink(char const *target) {
    wrlock_inode(ROOT_DIR_INUM);

    inode_t *root = inode_get(ROOT_DIR_INUM);

    int inum = find_in_dir(root, target + 1);
    if (inum < 0) {
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    wrlock_inode(inum);

    inode_t *inode = inode_get(inum);
    if (!inode) {
        unlock_inode(inum);
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    if (clear_dir_entry(root, target + 1) == -1) {
        unlock_inode(inum);
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    if (--inode->i_links > 0) {
        unlock_inode(inum);
        unlock_inode(ROOT_DIR_INUM);
        return 0;
    }

    if (find_open_file_entry(inum) < 0) {
        inode_delete(inum);
    }

    unlock_inode(inum);
    unlock_inode(ROOT_DIR_INUM);
    return 0;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    char buffer[state_block_size()];

    FILE *file = fopen(source_path, "r");
    if (!file) {
        return -1;
    }

    size_t to_write = fread(buffer, 1, state_block_size(), file);
    if (to_write == EOF) {
        return -1;
    }

    int fhandle = tfs_open(dest_path, TFS_O_CREAT | TFS_O_TRUNC);
    if (fhandle == -1) {
        return -1;
    }

    return (tfs_write(fhandle, buffer, to_write) >= 0) - 1;
}
