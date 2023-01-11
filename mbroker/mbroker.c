#include "../fs/operations.h"
#include "../utils/channel.h"
#include "../utils/logging.h"
#include "../utils/thread.h"
#include "../utils/box.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static Box boxes[MAX_BOX_COUNT];

static char *reg_channel_name;
static int max_sessions;

void worker(void);

void register_pub(char *channel_name, char *box_name);
void register_sub(char *channel_name, char *box_name);
void create_box(char *channel_name, char *box_name);
void remove_box(char *channel_name, char *box_name);
void list_boxes(char *channel_name);

int main(int argc, char **argv) {
    (void)boxes;
    if(argc < 3) {
        fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions> \n");
        return EXIT_SUCCESS;
    }

    reg_channel_name = argv[1];
    max_sessions = atoi(argv[2]);

    {
        tfs_params params = tfs_default_params();

        params.max_inode_count = MAX_BOX_COUNT;
        params.max_open_files_count = (size_t) max_sessions;

        tfs_init(&params);
    }

    channel_create(reg_channel_name, 0640);

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
        case REGISTER_PUB: {
            char channel_name[MAX_CHANNEL_NAME_SIZE];
            char box_name[MAX_BOX_NAME_SIZE];

            channel_read_content(fd, code, channel_name, box_name);

            register_pub(channel_name, box_name);
        } break;
        case REGISTER_SUB: {
            char channel_name[MAX_CHANNEL_NAME_SIZE];
            char box_name[MAX_BOX_NAME_SIZE];

            channel_read_content(fd, code, channel_name, box_name);

            register_sub(channel_name, box_name);
        } break;
        case CREATE_BOX: {
            char channel_name[MAX_CHANNEL_NAME_SIZE];
            char box_name[MAX_BOX_NAME_SIZE];

            channel_read_content(fd, code, channel_name, box_name);

            create_box(channel_name, box_name);
        } break;
        case REMOVE_BOX: {
            char channel_name[MAX_CHANNEL_NAME_SIZE];
            char box_name[MAX_BOX_NAME_SIZE];

            channel_read_content(fd, code, channel_name, box_name);

            remove_box(channel_name, box_name);
        } break;
        case LIST_BOXES: {
            char channel_name[MAX_CHANNEL_NAME_SIZE];

            channel_read_content(fd, code, channel_name);
            
            list_boxes(channel_name);
        } break;
        default:
            // invalid operation code
            break;
    }

    channel_close(fd);
}

void register_pub(char *channel_name, char *box_name) {
    // Publisher: session started.

    char message[MAX_MESSAGE_SIZE];

    uint8_t code;
    int fd = channel_open(channel_name, O_RDONLY);
    int fhandle = tfs_open(box_name, TFS_O_APPEND);

    // Keep session while channel is open (code != 0).
    while ((code = channel_read_code(fd))) {
        assert(code == MSG_PUB_TO_SER);
        channel_read_content(fd, code, message);
        tfs_write(fhandle, message, strlen(message)+1);
    }

    tfs_close(fhandle);

    // Publisher: session terminated.
}

void register_sub(char *channel_name, char *box_name) {
    // Subscriber: session started.

    char message[MAX_MESSAGE_SIZE];

    int fd = channel_open(channel_name, O_WRONLY);
    int fhandle = tfs_open(box_name, TFS_O_CREAT);

    while (tfs_read(fhandle, message, MAX_MESSAGE_SIZE) != -1) {
        channel_write(fd, MSG_SER_TO_SUB, message);
    }

    tfs_close(fhandle);
    channel_close(fd);

    // Subscriber: session terminated.
}

void create_box(char *channel_name, char *box_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    channel_close(fd);

    // Manager: session terminated.
}

void remove_box(char *channel_name, char *box_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    channel_close(fd);

    // Manager: session terminated.
}

void list_boxes(char *channel_name) {
    // Manager: session started.

    int fd = channel_open(channel_name, O_WRONLY);

    channel_close(fd);

    // Manager: session terminated.
}
