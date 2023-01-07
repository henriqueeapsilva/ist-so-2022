#include "utils/channel.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static enum resquest_t { CREATE=3, REMOVE=5, LIST=7, UNDEF=-1 };

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe> <pipe_name> list\n");
}

static uint8_t eval_request(int argc, char **argv) {
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
    uint8_t code = eval_request(argc, argv);

    if(code == UNDEF) {
        print_usage();
        return EXIT_SUCCESS;
    }

    create_channel(argv[2], 0640);

    int fregister = open_channel(argv[1], O_WRONLY);
    int fsession = open_channel(argv[2], O_WRONLY);

    switch (code) {
        case CREATE:
            // send create request

            memwrite_to_channel(fregister, &code);
            strwrite_to_channel(fregister, argv[2], 256);
            strwrite_to_channel(fregister, argv[4], 32);

            // receive response

            read_from_channel(fsession, &code, sizeof(uint8_t));
            assert(code == 4);

            int32_t failed;

            read_from_channel(fsession, &failed, sizeof(int32_t));

            if (failed) {
                char error_message[1024];
                read_from_channel(fsession, error_message, sizeof(error_message));
                fprintf(stdout, "ERROR %s\n", error_message);
            } else {
                fprintf(stdout, "OK\n");
            }

            break;
        case REMOVE:
            // send remvove request

            memwrite_to_channel(fregister, &code);
            strwrite_to_channel(fregister, argv[2], 256);
            strwrite_to_channel(fregister, argv[4], 32);

            // receive response

            read_from_channel(fsession, &code, sizeof(uint8_t));
            assert(code == 6);

            int32_t failed;

            read_from_channel(fsession, &failed, sizeof(int32_t));

            if (failed) {
                char error_message[1024];
                read_from_channel(fsession, error_message, sizeof(error_message));
                fprintf(stdout, "ERROR %s\n", error_message);
            } else {
                fprintf(stdout, "OK\n");
            }

            break;
        case LIST:
            // send list request

            memwrite_to_channel(fregister, &code);
            strwrite_to_channel(fregister, argv[2], 256);

            // receive response

            read_from_channel(fsession, &code, sizeof(uint8_t));

            assert(code == 8);

            int32_t last;
            char box_name[32];

            read_from_channel(fsession, &last, sizeof(uint8_t));
            read_from_channel(fsession, &box_name, sizeof(box_name));

            if (last && (box_name[0] == '\0')) {
                fprintf(stdout, "NO BOXES FOUND\n");
            } else {
                while (1) {
                    // TODO: sort boxes alphabetically

                    uint64_t box_size;
                    uint64_t n_publishers;
                    uint64_t n_subscribers;

                    read_from_channel(fsession, &box_size, sizeof(box_size));
                    read_from_channel(fsession, &n_publishers, sizeof(n_publishers));
                    read_from_channel(fsession, &n_subscribers, sizeof(n_subscribers));

                    fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers, n_subscribers);

                    if (last) break;

                    read_from_channel(fsession, &last, sizeof(uint8_t));
                    read_from_channel(fsession, &box_name, sizeof(box_name));
                }
            }

            break;
    }

    close_channel(fregister);
    close_channel(fsession);
    delete_channel(argv[2]);

    return EXIT_SUCCESS;
}
