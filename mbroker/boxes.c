#include "boxes.h"
#include "../fs/operations.h"
#include "../utils/channel.h"
#include "../utils/protocol.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_BOX_COUNT (params.max_inode_count)

static tfs_params params;
static Box *boxes;

int init_boxes() {
    params = tfs_default_params();
    boxes = (Box *)malloc(sizeof(Box) * MAX_BOX_COUNT);

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
            return (boxes + i);
        }
    }

    return NULL;
}

static Box *find_box(char *box_name) {
    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (!strcmp(box_name, boxes[i].name)) {
            return (boxes + i);
        }
    }

    return NULL;
}

void create_box(char *channel_name, char *box_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 4;
    char buffer[2048];

    Box *box = next_box();

    if (!box) {
        serialize_message(buffer, code, -1, "Unable to create box: no space available.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    int fhandle = tfs_open(box_name, TFS_O_CREAT);

    if (fhandle == -1) {
        serialize_message(buffer, code, -1, "Unable to delete box: tfs_open returned an error.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    tfs_close(fhandle);

    strcpy(box->name, box_name);
    box->size = 0;
    box->n_pubs = 0;
    box->n_subs = 0;
    serialize_message(buffer, code, 0, "");

    channel_write(fd, buffer, sizeof(buffer));
    channel_close(fd);

    // Manager: session terminated.
}

void remove_box(char *channel_name, char *box_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 6;
    char buffer[2048];

    Box *box = find_box(box_name);

    if (!box) {
        serialize_message(buffer, code, -1, "Unable to remove box: box not found.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    if (tfs_unlink(box_name) == -1) {
        serialize_message(buffer, code, -1,
                      "Unable to remove box: tfs_unlink returned an error.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    serialize_message(buffer, code, 0, "");
    channel_write(fd, buffer, sizeof(buffer));
    channel_close(fd);

    // Manager: session terminated.
}

void list_boxes(char *channel_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 8;
    uint8_t last = 1;
    char buffer[2048];

    Box *box = (boxes + MAX_BOX_COUNT);

    while ((--box >= boxes) && !(*box->name));

    if (box < boxes) {
        uint64_t dummy = 0;
        serialize_message(buffer, code, &last, "", &dummy, &dummy, &dummy);
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    } else {
        serialize_message(buffer, code, &last, box->name, &box->size, &box->n_pubs, &box->n_subs);
        channel_write(fd, buffer, sizeof(buffer));
        last = 0;
    }

    while (--box >= boxes) {
        if (*box->name) {
            serialize_message(buffer, code, &last, box->name, &box->size, &box->n_pubs,
                          &box->n_subs);
            channel_write(fd, buffer, sizeof(buffer));
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

    uint8_t code = OP_MSG_PUB_TO_SER;
    char buffer[2048];
    char message[1024];

    while (channel_read(fd, buffer, sizeof(buffer))) {
        assert(deserialize_code(buffer) == code);
        deserialize_message(buffer, code, message);

        box->size += (uint64_t) tfs_write(fhandle, message, strlen(message) + 1);
    }

    tfs_close(fhandle);
    channel_close(fd);

    box->n_pubs = 0;

    // Publisher: session terminated.
}

void register_sub(char *channel_name, char *box_name) {
    // Subscriber: session started.

    Box *box = find_box(box_name);

    if (!box) {
        return;
    }

    int fd = channel_open(channel_name, O_WRONLY);
    int fhandle = tfs_open(box_name, TFS_O_CREAT);

    uint8_t code = 10;
    char buffer1[2048]; // file buffer
    char buffer2[2048]; // channel buffer

    int offset = 0;
    int towrite = (int) tfs_read(fhandle, buffer1, sizeof(buffer1));

    while (1) {
        while (towrite >= 0) {
            serialize_message(buffer2, code, buffer1 + offset);
            channel_write(fd, buffer2, sizeof(buffer2));

            int len = (int) strlen(buffer2) + 1;

            offset += len;
            towrite -= (ssize_t)len;
        }

        // TODO: block until more messages are published.
    }

    tfs_close(fhandle);
    channel_close(fd);

    // Subscriber: session terminated.
}