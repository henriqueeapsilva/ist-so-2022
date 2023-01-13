#include "boxes.h"
#include "../fs/operations.h"
#include "../utils/channel.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

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
        boxes[i].name[0] = '\0';
    }

    return tfs_init(&params);
}

int destroy_boxes() {
    free(boxes);
    
    return tfs_destroy();
}

static int find_box(char *box_name) {
    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (!strcmp(box_name, boxes[i].name)) {
            return i;
        }
    }

    return -1;
}

void create_box(char *channel_name, char *box_name) {
    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 4;

    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (boxes[i].name[0]) {
            continue;
        }
        
        int fhandle = tfs_open(box_name, TFS_O_CREAT);

        if (fhandle == -1) {
            channel_write(fd, code, -1 ,"Unable to delete box: tfs_open returned an error.");
            channel_close(fd);
            return;
        }

        tfs_close(fhandle);

        strcpy(boxes[i].name, box_name);
        boxes[i].size = 0;
        boxes[i].n_pubs = 0;
        boxes[i].n_subs = 0;
        channel_write(fd, code, 0, "");
        channel_close(fd);
        return;
    }

    channel_write(fd, code, -1, "Unable to create box: no space available.");
    channel_close(fd);
}

void remove_box(char *channel_name, char *box_name) {
    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 6;

    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (!strcmp(boxes[i].name, box_name)) {
            continue;
        }

        if(tfs_unlink(box_name) == -1){
            channel_write(fd, code, -1 ,"Unable to remove box: tfs_unlink returned an error.");
            channel_close(fd);
            return;
        }

        channel_write(fd, code, 0, "");
        channel_close(fd);
        return;
    }

    channel_write(fd, code, -1 ,"Unable to remove box: box not found.");
    channel_close(fd);
}

void list_boxes(char *channel_name) {
    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 8;
    uint8_t last = 1;
    
    Box *box = &boxes[MAX_BOX_COUNT-1];

    while (box >= boxes) {
        if (box->name[0]) {
            channel_write(fd, code, last, box->name, box->size, box->n_pubs, box->n_subs);
            last = 0;
            break;
        }

        box--;
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
}

void register_pub(char *channel_name, char *box_name) {
    int bnum = find_box(box_name);

    if ((bnum == -1) || boxes[bnum].n_pubs) {
        return;
    }

    boxes[bnum].n_pubs = 1;

    int fd = channel_open(channel_name, O_RDONLY);
    int fhandle = tfs_open(box_name, TFS_O_APPEND);

    uint8_t code;

    while ((code = channel_read_code(fd))) {
        assert(code == 9);

        char buffer[1024];

        channel_read_content(fd, code, buffer);

        boxes[bnum].size += tfs_write(fhandle, buffer, strlen(buffer)+1);
    }

    tfs_close(fhandle);
    channel_close(fd);

    boxes[bnum].n_pubs = 0;
}

void register_sub(char *channel_name, char *box_name) {
    int bnum = find_box(box_name);

    if (bnum == -1) {
        return;
    }

    int fd = channel_open(channel_name, O_WRONLY);
    int fhandle = tfs_open(box_name, TFS_O_CREAT);

    uint8_t code = 10;

    char buffer[BUF_LEN];

    int offset = 0;
    int towrite = 0;

    towrite = tfs_read(fhandle, buffer, BUF_LEN);

    while (1) {
        while (towrite >= 0) {
            channel_write(fd, code, buffer+offset);

            size_t len = strlen(buffer)+1;
    
            offset += len;
            towrite -= len;
        }

        // block until more messages are published
    }

    tfs_close(fhandle);
    channel_close(fd);
}