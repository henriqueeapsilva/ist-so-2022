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

void create_box(char *channel_name, char *box_name) {
    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t ret_code = 4;
    int32_t created = 0;

    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (boxes[i].name[0]) {
            continue;
        }
        
        int fhandle = tfs_open(box_name, TFS_O_CREAT);

        if (fhandle == -1) {
            channel_write(fd, ret_code, -1 , "Unable to create box: tfs not working at the moment");
        }

        tfs_close(fhandle);

        strcpy(boxes[i].name, box_name);
        boxes[i].size = 0;
        boxes[i].n_pubs = 0;
        boxes[i].n_subs = 0;
        created = 1;
        break;
    }

    if (created) {
        channel_write(fd, ret_code, -1, "Unable to create box: no space available.");
    } else {
        channel_write(fd, ret_code, 0);
    }


    channel_close(fd);
}

void remove_box(char *channel_name, char *box_name) {
    int fd = channel_open(channel_name, O_WRONLY);

    uint8_t ret_code = 6;
    int32_t failed = -1;

    for (int i = 0; i < MAX_BOX_COUNT; i++) {
        if (!strcmp(boxes[i].name,box_name)) {
            continue;
        }

        failed = 0;

        if(tfs_unlink(box_name) == -1){
            channel_write(fd, ret_code, -1 ,"Unable to delete box.");     
        }
        break;
    }

    if (failed) {
        channel_write(fd, ret_code, -1 ,"Unable to create box: no space available.");
    }

    channel_close(fd);
}

void list_boxes(char *channel_name) {

}

void register_pub(char *channel_name) {

}

void register_sub(char *channel_name) {

}