#include "boxes.h"
#include "../fs/operations.h"
#include "../utils/channel.h"
#include "../utils/protocol.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../utils/logging.h"

static tfs_params params;
static Box *boxes;

#define MAX_BOX_COUNT (params.max_inode_count)

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
    Box *end = boxes+MAX_BOX_COUNT;

    for (Box *box = boxes; box < end; box++) {
        if (!box->name[0]) {
            return box;
        }
    }

    return NULL;
}

static Box *find_box(char *box_name) {
    Box *end = boxes+MAX_BOX_COUNT;

    for (Box *box = boxes; box < end; box++) {
        if (!strcmp(box_name, box->name)) {
            return box;
        }
    }

    return NULL;
}

void create_box(char *channel_name, char *box_name) {
    LOG("Manager: creating box (%s)...", box_name);

    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = OP_CREATE_BOX_RET;
    int32_t ret_code = -1;
    char buffer[2048];

    if (find_box(box_name)) {
        LOG("Manager: box already exists");
        serialize_message(buffer, code, &ret_code, "Unable to create box: box already exists.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    Box *box = next_box();

    if (!box) {
        LOG("Manager: could not create box.");
        serialize_message(buffer, code, &ret_code, "Unable to create box: no space available.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    char filename[MAX_FILE_NAME];
    sprintf(filename, "/%s", box_name);

    int fhandle = tfs_open(filename, TFS_O_CREAT);

    if (fhandle == -1) {
        LOG("Manager: could not create box.");
        serialize_message(buffer, code, &ret_code, "Unable to create box: tfs_open returned an error.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    tfs_close(fhandle);

    strcpy(box->name, box_name);
    box->size = 0;
    box->n_pubs = 0;
    box->n_subs = 0;
    ret_code = 0;
    serialize_message(buffer, code, &ret_code, "");

    channel_write(fd, buffer, sizeof(buffer));
    channel_close(fd);

    LOG("Manager: box created.");
}

void remove_box(char *channel_name, char *box_name) {
    LOG("Manager: removing box (%s)...", box_name);

    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 6;
    int32_t ret_code = 0;
    char buffer[2048];

    Box *box = find_box(box_name);

    if (!box) {
        LOG("Manager: could not remove box.");
        ret_code = -1;
        serialize_message(buffer, code, &ret_code, "Unable to remove box: box not found.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    char filename[MAX_FILE_NAME];
    sprintf(filename, "/%s", box_name);

    if (tfs_unlink(filename) == -1) {
        LOG("Manager: could not remove box.");
        ret_code = -1;
        serialize_message(buffer, code, &ret_code,
                      "Unable to remove box: tfs_unlink returned an error.");
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    box->name[0] = 0;

    serialize_message(buffer, code, &ret_code, "");
    channel_write(fd, buffer, sizeof(buffer));
    channel_close(fd);

    LOG("Manager: box removed.")
}

void list_boxes(char *channel_name) {
    int fd = channel_open(channel_name, O_WRONLY);
    LOG("Manager: listing boxes...");

    uint8_t code = 8;
    uint8_t last = 0;
    char buffer[2048];

    Box *box;
    Box *curr = boxes;
    Box *end = curr + MAX_BOX_COUNT;

    while ((curr < end) && (!*curr->name));

    if (curr == end) {
        LOG("Manager: no boxes to list.");
        uint64_t dummy = 0;
        serialize_message(buffer, code, &last, "", &dummy, &dummy, &dummy);
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        return;
    }

    box = curr;
    while (curr < end) {
        if (*curr->name) {
            serialize_message(buffer, code, &last, box->name, &box->size, &box->n_pubs,
                          &box->n_subs);
            channel_write(fd, buffer, sizeof(buffer));
            box = curr;
        }
    }

    last = 1;
    serialize_message(buffer, code, &last, box->name, &box->size, &box->n_pubs,
                          &box->n_subs);
    channel_write(fd, buffer, sizeof(buffer));

    channel_close(fd);
    LOG("Manager: boxes listed.");
}

void register_pub(char *channel_name, char *box_name) {
    DEBUG("Starting session...");
    int fd = channel_open(channel_name, O_RDONLY);
    DEBUG("Session started.");

    Box *box = find_box(box_name);

    if (!box) {
        DEBUG("Session terminated: box not found.");
        channel_close(fd);
        return;
    }

    if (box->n_pubs) {
        DEBUG("Session terminated: only 1 publisher supported.");
        channel_close(fd);
        return;
    }

    box->n_pubs = 1;

    char filename[MAX_FILE_NAME];
    sprintf(filename, "/%s", box_name);

    int fhandle = tfs_open(filename, TFS_O_APPEND);

    if (fhandle == -1) {
        DEBUG("Session terminated: tfs_open returned an error.");
        channel_close(fd);
        return;
    }

    uint8_t code = OP_MSG_PUB_TO_SER;
    char buffer[2048];
    char message[1024];

    // To catch the verification bite
    channel_read(fd, buffer, sizeof(buffer));

    while (channel_read(fd, buffer, sizeof(buffer))) {
        DEBUG("Message received.");

        assert(deserialize_code(buffer) == code);
        deserialize_message(buffer, code, message);

        ssize_t ret = tfs_write(fhandle, message, strlen(message)+1);

        if (ret == -1) {
            DEBUG("Session terminated: tfs_write returned an error.");
            channel_close(fd);
            return;
        }

        box->size += (uint64_t) ret;
        DEBUG("Message published.");
    }

    box->n_pubs = 0;

    tfs_close(fhandle);
    channel_close(fd);

    DEBUG("Session terminated.");
}

void register_sub(char *channel_name, char *box_name) {
    DEBUG("Starting session...");
    int fd = channel_open(channel_name, O_WRONLY);
    DEBUG("Session started.");

    Box *box = find_box(box_name);

    if (!box) {
        DEBUG("Session terminated: box not found.")
        channel_close(fd);
        return;
    }

    char filename[MAX_FILE_NAME];
    sprintf(filename, "/%s", box_name);

    int fhandle = tfs_open(filename, TFS_O_CREAT);

    if (fhandle == -1) {
        DEBUG("Session terminated: tfs_open returned an error.");
        channel_close(fd);
        return;
    }

    uint8_t code = 10;
    char block[1024];
    char buffer[2048];
    char *message;

    while (1) {
        ssize_t towrite = tfs_read(fhandle, block, sizeof(block));

        if (towrite == -1) {
            DEBUG("Session terminated: tfs_read returned an error.");
            channel_close(fd);
            return;
        }

        if (towrite == 0) {
            continue;
        }

        DEBUG("Characters found inside the box: %ld", towrite);

        message = block;
        while (towrite > 0) {
            serialize_message(buffer, code, message);
            channel_write(fd, buffer, sizeof(buffer));

            size_t len = strlen(block) + 1;

            message += len;
            towrite -= (ssize_t) len;
        }

        // TODO: block until more messages are published.
    }

    //tfs_close(fhandle);
    //channel_close(fd);

    //DEBUG("Session terminated.")
}