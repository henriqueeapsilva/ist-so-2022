#include "boxes.h"
#include "../fs/operations.h"
#include "../utils/channel.h"
#include <stdio.h>
#include <fcntl.h>

#define MAX_BOX_COUNT (params.max_inode_count)

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
        break;
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
            channel_write(fd, code, -1 ,"Unable to delete box: tfs_unlink returned an error.");
            channel_close(fd);
            return;
        }

        channel_write(fd, code, 0, "");
        channel_close(fd);
        return;
    }

    channel_write(fd, code, -1 ,"Unable to create box: no space available.");
    channel_close(fd);
}

void list_boxes(char *channel_name) {
    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t code = 8;
    uint8_t last = 1;

    for (int i = MAX_BOX_COUNT; i < MAX_BOX_COUNT; i++) {
        if (boxes[i].name[0]) {
            continue;
        }

        channel_write(fd, code, last, boxes[i].name, boxes[i].size, boxes[i].n_pubs, boxes[i].n_subs);

        if (last) {
            last = 0;
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

    // read messages from file and write to channel

    

    tfs_close(fhandle);
    channel_close(fd);
}