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

int main(int argc, char **argv) {
    if(argc < 3) {
        fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions> \n");
        return EXIT_SUCCESS;
    }

    reg_channel_name = argv[1];
    max_sessions = atoi(argv[2]);

    {
        tfs_params params = tfs_default_params();

        params.max_inode_count = MAX_BOX_COUNT;
        params.max_open_files_count = max_sessions;

        tfs_init(&params);
    }

    create_channel(reg_channel_name, 0640);

    while (1) {
        /* blocks until there is some writer available */
        int freg = open_channel(reg_channel_name, O_RDONLY);

        uint8_t code;
        while (code = read_code(freg)) {
            switch (code) {
                case 1: { /* register publisher */
                    char channel_name[MAX_CHANNEL_NAME_SIZE];
                    char box_name[MAX_BOX_NAME_SIZE];

                    read_content(freg, code, channel_name, box_name);

                    /* publisher: session started */

                    char buffer[MAX_MESSAGE_SIZE];

                    int fhandle = tfs_open(box_name, TFS_O_APPEND);
                    int fsession = open_channel(channel_name, O_RDONLY);

                    while (code = read_code(fsession)) {
                        read_content(fsession, code, buffer);
                        tfs_write(fhandle, buffer, strlen(buffer)+1);
                    }

                    tfs_close(fhandle);
                    close_channel(fsession);

                    /* publisher: session terminated */
                } break;
                case 2: { /* register subscriber */
                    char channel_name[MAX_CHANNEL_NAME_SIZE];
                    char box_name[MAX_BOX_NAME_SIZE];

                    read_content(freg, code, channel_name, box_name);

                    /* subscriber: session started */

                    char buffer[MAX_MESSAGE_SIZE];

                    int fhandle = tfs_open(box_name, TFS_O_CREAT);

                    /* subscriber: session terminated */
                } break;
                case 3: /* create box */
                case 5: /* remove box */
                case 7: /* list boxes */
                default: /* invalid request */
                    break;
            };
        }

        close_channel(freg);
    }
    
    delete_channel(reg_channel_name);

    tfs_destroy();

    return EXIT_SUCCESS;
}
