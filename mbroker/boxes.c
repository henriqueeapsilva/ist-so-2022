#include "boxes.h"
#include "../fs/operations.h"
#include "../utils/channel.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#define MAX_BOX_COUNT (params.max_inode_count)
#define BUF_LEN (1024)

static tfs_params params;
static Box *boxes;

int init_boxes() {
    params = tfs_default_params();
    boxes = (Box*) malloc(sizeof(Box)*MAX_BOX_COUNT);

    if (!boxes) {
        return -1;
    }

    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        boxes[i].name[0] = 0;
    }

    return tfs_init(&params);
}

int destroy_boxes() {
    free(boxes);
    
    return tfs_destroy();
}

/**
 * Returns the next free box available to be used.
 */
static Box *next_box() {
    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (!boxes[i].name[0]) {
            return (boxes+i);
        }
    }

    return NULL;
}

static Box *find_box(char *box_name) {
    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (!strcmp(box_name, boxes[i].name)) {
            return (boxes+i);
        }
    }

    return NULL;
}

void create_box(char *channel_name, char *box_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 4;

    Box *box = next_box();

    if (!box) {
        channel_write(fd, code, -1, "Unable to create box: no space available.");
        channel_close(fd);
        return;
    }

    int fhandle = tfs_open(box_name, TFS_O_CREAT);

    if (fhandle == -1) {
        channel_write(fd, code, -1 ,"Unable to delete box: tfs_open returned an error.");
        channel_close(fd);
        return;
    }

    tfs_close(fhandle);

    strcpy(box->name, box_name);
    box->size = 0;
    box->n_pubs = 0;
    box->n_subs = 0;
    channel_write(fd, code, 0, "");
    channel_close(fd);
    
    // Manager: session terminated.
}

void remove_box(char *channel_name, char *box_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 6;

    Box *box = find_box(box_name);

    if (!box) {
        channel_write(fd, code, -1 ,"Unable to remove box: box not found.");
        channel_close(fd);
        return;
    }

    if (tfs_unlink(box_name) == -1){
        channel_write(fd, code, -1 ,"Unable to remove box: tfs_unlink returned an error.");
        channel_close(fd);
        return;
    }

    channel_write(fd, code, 0, "");
    channel_close(fd);

    // Manager: session terminated.
}

void list_boxes(char *channel_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 8;
    uint8_t last = 1;
    
    Box *box = (boxes+MAX_BOX_COUNT);

    while (--box >= boxes) {
        if (box->name[0]) {
            channel_write(fd, code, last, box->name, box->size, box->n_pubs, box->n_subs);
            last = 0;
            break;
        }
    }

    if (last) {
        channel_write(fd, code, last, "", 0, 0, 0);
        channel_close(fd);
        return;
    }

    while (--box >= boxes) {
        if (box->name[0]) {
            channel_write(fd, code, last, box->name, box->size, box->n_pubs, box->n_subs);
        }
    }

    channel_close(fd);

    // Manager: session terminated.
}

void register_pub(char *channel_name, char *box_name) {
    // Publisher: session started.

    Box *box = find_box(box_name);

    if (!box || box->n_pubs) {
        return;
    }

    box->n_pubs = 1;

    int fd = channel_open(channel_name, O_RDONLY);
    int fhandle = tfs_open(box_name, TFS_O_APPEND);

    uint8_t code;

    while ((code = channel_read_code(fd))) {
        assert(code == 9);

        char buffer[1024];

        channel_read_content(fd, code, buffer);

        box->size += (uint64_t)tfs_write(fhandle, buffer, strlen(buffer)+1);
    }

    tfs_close(fhandle);
    channel_close(fd);

    box->n_pubs = 0;

    // Publisher: session terminated.
}

void register_sub(char *channel_name, char *box_name) {
    // Subscriber: session started.

    Box* box = find_box(box_name);

    if (!box) {
        return;
    }

    int fd = channel_open(channel_name, O_WRONLY);
    int fhandle = tfs_open(box_name, TFS_O_CREAT);

    uint8_t code = 10;

    char buffer[BUF_LEN];

    size_t offset = 0;
    ssize_t towrite = 0;

    towrite = tfs_read(fhandle, buffer, BUF_LEN);

    while (1) {
        while (towrite >= 0) {
            channel_write(fd, code, buffer+offset);

            size_t len = strlen(buffer)+1;
    
            offset += len;
            towrite -= (ssize_t)len;
        }

        // TODO: block until more messages are published.
    }

    tfs_close(fhandle);
    channel_close(fd);

    // Subscriber: session terminated.
}