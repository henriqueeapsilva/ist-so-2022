#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betterassert.h"

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

    if (state_init(params) != 0) return -1;

    // create root inode
    int root = inode_create(T_DIRECTORY);

    if (root != ROOT_DIR_INUM) return -1;

    return 0;
}

int tfs_destroy() {
    return state_destroy();
}

static bool valid_pathname(char const *name) {
    return (name != NULL) && (strlen(name) > 1) && (name[0] == '/');
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    if ((root_inode != inode_get(ROOT_DIR_INUM))
            && !valid_pathname(name)) return -1;

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) return -1;

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    int inum = tfs_lookup(name, root_dir_inode);

    if (inum == -1) { // node does not exist
        if (!(mode & TFS_O_CREAT)) return -1;

        inum = inode_create(T_FILE);

        if (inum == -1) return -1; // no space in inode table

        // add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            return -1; // no space in directory
        }

        return add_to_open_file_table(inum, 0);
    }

    inode_t *inode = inode_get(inum);
    ALWAYS_ASSERT(inode != NULL,
                  "tfs_open: directory files must have an inode");

    if (inode->i_node_type == T_LINK) {
        char *block = data_block_get(inode->i_data_block);
        if(tfs_lookup(block,root_dir_inode) == -1) return -1;
        return tfs_open(block, mode);
    }

    size_t offset = 0;
    
    // Truncate (if requested)
    if (mode & TFS_O_TRUNC) {
        if (inode->i_size > 0) {
            data_block_free(inode->i_data_block);
            inode->i_size = 0;
        }
    } else if (mode & TFS_O_APPEND) {
        offset = inode->i_size;
    }

    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    int inum = inode_create(T_LINK);

    if (inum == -1) return -1;

    if (add_dir_entry(inode_get(ROOT_DIR_INUM), link_name + 1, inum) == -1) {
        inode_delete(inum);
        return -1;
    }

    inode_t *inode = inode_get(inum);

    if (!inode) return -1;

    if (inode->i_size == 0) {
        inode->i_data_block = data_block_alloc();

        if (inode->i_data_block == -1) return -1;
    }

    char *block = data_block_get(inode->i_data_block);

    strcpy(block, target);

    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    if (!root_dir_inode) return -1;

    int inum = tfs_lookup(target, root_dir_inode);
    if (inum == -1) return -1;

    inode_t *inode = inode_get(inum);
    if (!inode) return -1;

    if (inode->i_node_type == T_LINK) return -1;

    if (add_dir_entry(root_dir_inode, link_name + 1, inum) == -1) return -1;

    inode->i_links++;
    return 0;
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);

    if (file == NULL) return -1;

    // From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            inode->i_data_block = data_block_alloc();

            if (inode->i_data_block == -1) return -1; // no space
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);

    if (file == NULL) return -1;

    // From the open file table entry, we get the inode
    inode_t const *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    int inum = tfs_lookup(target, root_dir_inode);

    if (inum == -1) return -1;

    inode_t *inode = inode_get(inum);

    if (!inode) return -1;

    if (clear_dir_entry(root_dir_inode, target+1) == -1) return -1;

    if ((--inode->i_links) == 0) {
        inode_delete(inum);
    }

    return 0;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    FILE *file = fopen(source_path, "r");
    if (!file) return -1;

    size_t chars = state_block_size();
    char buffer[chars];

    size_t result = fread(buffer, 1, chars, file);
        
    if (result == -1) {
        fclose(file);
        return -1;
    }

    if (fclose(file) == EOF) return -1;

    int fhandle = tfs_open(dest_path, TFS_O_TRUNC | TFS_O_CREAT);
    if (fhandle == -1) return -1;

    if (tfs_write(fhandle, buffer, result) < 0) {
        tfs_close(fhandle);
        return -1;
    }

    if (tfs_close(fhandle) == -1) return -1;

    return 0;
}
