#include "fifo.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static enum resquest_t { CREATE=3, REMOVE=5, LIST=7, UNDEF=-1 };

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe> <pipe_name> list\n");
}

static int eval_request(int argc, char **argv) {
    if (argc < 4) return UNDEF;

    if (!strcmp(argv[3], "create")) {
        if (argc < 5) return UNDEF;

        return CREATE;
    }

    if (!strcmp(argv[3], "remove")) {
        if (argc < 5) return UNDEF;

        return REMOVE;
    }

    return strcmp(argv[3], "list") ? UNDEF : LIST;
}

int main(int argc, char **argv) {
    int request = eval_request(argc, argv);

    if(request == UNDEF) {
        print_usage();
        return EXIT_SUCCESS;
    }

    // Create pipe
    fifo_make(argv[2], 0640);

    int fregister = fifo_open(argv[1], O_WRONLY);
    int fsession = fifo_open(argv[2], O_WRONLY);

    switch (request) {
        case CREATE:
            /*
             * Create request:
             * [ code = 3 (uint8_t) ] | [ session_pipe_name (char[256]) ] | [ box_name (char[32]) ]
             */

            char msg[1+256+32];

            sprintf(msg, "%d", request);     // insert request code
            strncpy(msg+1, argv[2], 256);    // insert session pipe name
            strncpy(msg+1+256, argv[4], 32); // insert box name

            fifo_send_msg(fregister, msg);

            break;
        case REMOVE:
            /*
             * Remove request:
             * [ code = 5 (uint8_t) ] | [ session_pipe_name (char[256]) ] | [ box_name (char[32]) ]
             */

            char msg[1+256+32];

            sprintf(msg, "%d", request);     // insert request code
            strncpy(msg+1, argv[2], 256);    // insert session pipe name
            strncpy(msg+1+256, argv[4], 32); // insert box name

            fifo_send_msg(fregister, msg);
            
            break;
        case LIST:
            /*
             * List request:
             * [ code = 7 (uint8_t) ] | [ session_pipe_name (char[256]) ]
             */

            char msg[1+256];

            sprintf(msg, "%d", request);     // insert request code
            strncpy(msg+1, argv[2], 256); // insert session pipe name

            fifo_send_msg(fregister, msg);

            break;
        default:
            // should not happen
    }

    fifo_close(fregister);
    fifo_close(fsession);
    fifo_unlink(argv[2]);

    fregister = NULL;
    fsession = NULL;
    
    return EXIT_SUCCESS;
}
