#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betterassert.h"

#define ROOT_DIR_INODE (inode_get(ROOT_DIR_INUM))

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
  int root = inode_create(T_DIRECTORY);
  if (root != ROOT_DIR_INUM) {
    return -1;
  }

  return 0;
}

int tfs_destroy() {
  return state_destroy();
}

static bool valid_pathname(char const *name) {
  return (name != NULL) && (strlen(name) > 1) && (name[0] == '/');
}

static int tfs_create(char const *name, inode_type type) {
  if (!valid_pathname(name)) {
    return -1;
  }

  int inum = inode_create(type);
  if (inum < 0) {
    return -1;
  }

  if (add_dir_entry(ROOT_DIR_INODE, ++name, inum) == -1) {
    inode_delete(inum);
    return -1;
  }

  return inum;
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
  if (!valid_pathname(name)) {
    return -1;
  }

  int inum = find_in_dir(ROOT_DIR_INODE, name+1);
  size_t offset = 0;
  inode_t *inode = NULL;

  if (inum >= 0) {
    inode = inode_get(inum);
    ALWAYS_ASSERT(inode != NULL, "tfs_open: directory files must have an inode");

    if (inode->i_node_type == T_LINK) {
      char *block = data_block_get(inode->i_data_block);
      if (!block) {
        return -1;
      }

      return tfs_open(block, mode);
    }
  }

  // Truncate
  if ((mode & TFS_O_TRUNC) && inode && (inode->i_size > 0)) {
    inode_free(inum);
  }

  // Append
  else if ((mode & TFS_O_APPEND) && inode) {
    offset = inode->i_size;
  }

  // Create
  else if ((mode & TFS_O_CREAT) && !inode) {
    inum = tfs_create(name, T_FILE);
  }

  return (inum < 0) ? -1 : add_to_open_file_table(inum, offset);

  // Note: for simplification, if file was created with TFS_O_CREAT and there
  // is an error adding an entry to the open file table, the file is not
  // opened but it remains created
}

int tfs_close(int fhandle) {
  open_file_entry_t *file = get_open_file_entry(fhandle);
  if (file == NULL) {
    return -1;
  }

  remove_from_open_file_table(fhandle);
  return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
  open_file_entry_t *file = get_open_file_entry(fhandle);
  if (file == NULL) {
    return -1;
  }

  inode_t *inode = inode_get(file->of_inumber);
  ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

  // Determine how many bytes to write
  size_t block_size = state_block_size();
  if ((to_write + file->of_offset) > block_size) {
    to_write = (block_size - file->of_offset);
  }

  if (to_write > 0) {
    if (inode->i_size == 0) {
      inode->i_data_block = data_block_alloc();
      if (inode->i_data_block == -1) {
        return -1; // no space
      }
    }

    void *block = data_block_get(inode->i_data_block);
    ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

    memcpy((block + file->of_offset), buffer, to_write);

    file->of_offset += to_write;
    if (file->of_offset > inode->i_size) {
      inode->i_size = file->of_offset;
    }
  }

  return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
  open_file_entry_t *file = get_open_file_entry(fhandle);
  if (file == NULL) {
    return -1;
  }

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
    memcpy(buffer, (block + file->of_offset), to_read);
    // The offset associated with the file handle is incremented accordingly
    file->of_offset += to_read;
  }

  return (ssize_t)to_read;
}

int tfs_sym_link(char const *target, char const *link_name) {
  if (!valid_pathname(target)) {
    return -1;
  }

  if (strcmp(target, link_name) == 0) {
    return -1;
  }

  if (strlen(target) > state_block_size()) {
    return -1;
  }

  int inum = tfs_create(link_name, T_LINK);
  if (inum == -1) {
    return -1;
  }

  inode_t *inode = inode_get(inum);
  if (!inode) {
    return -1;
  }

  inode->i_data_block = data_block_alloc();
  if (inode->i_data_block == -1) {
    return -1;
  }

  char *block = data_block_get(inode->i_data_block);
  if (!block) {
    return -1;
  }

  strncpy(block, target, MAX_FILE_NAME-1);
  block[MAX_FILE_NAME-1] = '\0';
  return 0;
}

int tfs_link(char const *target, char const *link_name) {
  int inum = find_in_dir(ROOT_DIR_INODE, target+1);
  if (inum == -1) {
    return -1;
  }

  inode_t *inode = inode_get(inum);
  if (!inode || (inode->i_node_type == T_LINK)) {
    return -1;
  }

  return add_dir_entry(ROOT_DIR_INODE, link_name+1, inum);
}

int tfs_unlink(char const *target) {
  return clear_dir_entry(ROOT_DIR_INODE, target+1);
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
  int bnum = data_block_alloc();
  if (bnum == -1) {
    return -1;
  }

  char *block = data_block_get(bnum);
  if (!block) {
    return -1;
  }

  FILE *file = fopen(source_path, "r");
  if (!file) {
    data_block_free(bnum);
    return -1;
  }

  size_t size = fread(block, sizeof(char), state_block_size(), file);
  if (size == -1) {
    data_block_free(bnum);
    return -1;
  }

  int inum = tfs_create(dest_path, T_FILE);
  if (inum == EOF) {
    data_block_free(bnum);
    return -1;
  }

  inode_t *inode = inode_get(inum);
  if (!inode) {
    data_block_free(bnum);
    return -1;
  }

  inode->i_data_block = bnum;
  inode->i_size = size;
  return 0;
}
