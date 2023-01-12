#include "../fs/operations.h"
#include "../utils/channel.h"
#include "../utils/logging.h"
#include "../utils/thread.h"
#include "boxes.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


static char *reg_channel_name;
static int max_sessions;

void worker(void);

int main(int argc, char **argv) {
    if(argc < 3) {
        fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions> \n");
        return EXIT_SUCCESS;
    }

    reg_channel_name = argv[1];
    max_sessions = atoi(argv[2]);

    channel_create(reg_channel_name, 0640);

    if(init_boxes()) return EXIT_FAILURE;

    while (1) {
        worker();
    }
    
    channel_delete(reg_channel_name);
    tfs_destroy();

    return EXIT_SUCCESS;
}

void worker() {
    int fd = channel_open(reg_channel_name, O_RDONLY);

    uint8_t code = channel_read_code(fd);
    switch (code) {
        case OP_REGISTER_PUB: {
            char channel_name[256];
            char box_name[32];

            channel_read_content(fd, code, channel_name, box_name);

            register_pub(channel_name, box_name);
        } break;
        case OP_REGISTER_SUB: {
            char channel_name[256];
            char box_name[32];

            channel_read_content(fd, code, channel_name, box_name);

            register_sub(channel_name, box_name);
        } break;
        case OP_CREATE_BOX: {
            char channel_name[256];
            char box_name[32];

            channel_read_content(fd, code, channel_name, box_name);

            create_box(channel_name, box_name);
        } break;
        case OP_REMOVE_BOX: {
            char channel_name[256];
            char box_name[32];

            channel_read_content(fd, code, channel_name, box_name);

            remove_box(channel_name, box_name);
        } break;
        case OP_LIST_BOXES: {
            char channel_name[256];

            channel_read_content(fd, code, channel_name);
            
            list_boxes(channel_name);
        } break;
        default:
            // invalid operation code
            break;
    }

    channel_close(fd);
}
