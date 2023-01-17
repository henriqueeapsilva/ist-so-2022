/* -- Includes -- */

#include "../fs/operations.h"
#include "../utils/pipe.h"
#include "../utils/logging.h"
#include "../utils/protocol.h"
#include "../utils/thread.h"
#include "boxes.h"
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -- Interface -- */

void worker(void);

/* -- Global Variables -- */

static char *reg_channel_name;
static int max_sessions;

/* -- Functions -- */

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr,
                "usage: mbroker <register_pipe_name> <max_sessions> \n");
        return EXIT_SUCCESS;
    }

    reg_channel_name = argv[1];
    max_sessions = atoi(argv[2]);

    if (init_boxes())
        return EXIT_FAILURE;

    pipe_create(reg_channel_name, 0640);

    while (1) {
        worker();
    }

    // pipe_delete(reg_channel_name);
    // tfs_destroy();

    return EXIT_SUCCESS;
}

void worker(void) {
    char buffer[2048];

    LOG("Waiting for some request.");
    int fd = pipe_open(reg_channel_name, O_RDONLY);
    pipe_read(fd, buffer, sizeof(buffer));
    pipe_close(fd);

    uint8_t code = deserialize_code(buffer);
    LOG("Request received. Code: %d", code);

    switch (code) {
    case OP_REGISTER_PUB: {
        char channel_name[256];
        char box_name[32];

        deserialize_message(buffer, code, channel_name, box_name);

        register_pub(channel_name, box_name);
    } break;
    case OP_REGISTER_SUB: {
        char channel_name[256];
        char box_name[32];

        deserialize_message(buffer, code, channel_name, box_name);

        register_sub(channel_name, box_name);
    } break;
    case OP_CREATE_BOX: {
        char channel_name[256];
        char box_name[32];

        deserialize_message(buffer, code, channel_name, box_name);

        create_box(channel_name, box_name);
    } break;
    case OP_REMOVE_BOX: {
        char channel_name[256];
        char box_name[32];

        deserialize_message(buffer, code, channel_name, box_name);

        remove_box(channel_name, box_name);
    } break;
    case OP_LIST_BOXES: {
        char channel_name[256];

        deserialize_message(buffer, code, channel_name);

        list_boxes(channel_name);
    } break;
    default:
        // invalid operation code
        break;
    }

    LOG("Finished request.");
}
