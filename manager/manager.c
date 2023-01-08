#include "../utils/channel.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static enum resquest_t { CREATE = 3, REMOVE = 5, LIST = 7, UNDEF = -1 };

static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

bool eval_request(int argc, char **argv);
void create_box(int argc, char **argv);
void remove_box(int argc, char **argv);
void list(int argc, char **argv);

int main(int argc, char **argv) {
    if (!eval_request(argc, argv)) {
        print_usage();
    }

    return EXIT_SUCCESS;
}

bool eval_request(int argc, char **argv) {
    if (argc < 4)
        return false;

    if (!strcmp(argv[3], "create")) {
        if (argc < 5)
            return false;

        create_box(argc, argv);
        return true;
    }

    if (!strcmp(argv[3], "remove")) {
        if (argc < 5)
            return false;

        remove_box(argc, argv);
        return true;
    }

    if (!strcmp(argv[3], "list")) {
        list(argc, argv);
        return true;
    }
}

void update_box(int argc, char **argv, uint8_t request) {
    // create channel

    create_channel(argv[2], 0640);

    // send request

    {
        int fregistry = open_channel(argv[1], O_WRONLY);

        memwrite_to_channel(fregistry, (uint8_t *)&request);
        strwrite_to_channel(fregistry, argv[2], 256);
        strwrite_to_channel(fregistry, argv[4], 32);

        close_channel(fregistry);
    }

    // receive response

    {
        int fsession = open_channel(argv[2], O_WRONLY);

        uint8_t code;
        read_from_channel(fsession, &code, sizeof(code));
        assert(code == (request + 1));

        int32_t failed;
        char message[1024];
        read_from_channel(fsession, &failed, sizeof(failed));
        read_from_channel(fsession, message, sizeof(message));

        if (failed) {
            fprintf(stdout, "ERROR %s\n", message);
        } else {
            fprintf(stdout, "OK\n");
        }

        close_channel(fsession);
    }

    // delete channel

    delete_channel(argv[2]);
}

void create_box(int argc, char **argv) { update_box(argc, argv, 3); }

void remove_box(int argc, char **argv) { update_box(argc, argv, 5); }

void list(int argc, char **argv) {
    // create channel

    create_channel(argv[2], 0640);

    // send request

    {
        int fregistry = open_channel(argv[1], O_WRONLY);

        uint8_t request = 7;
        memwrite_to_channel(fregistry, (uint8_t *)&request);
        strwrite_to_channel(fregistry, argv[2], 256);

        close_channel(fregistry);
    }

    // receive response

    {
        int fsession = open_channel(argv[2], O_WRONLY);

        uint8_t code;
        read_from_channel(fsession, &code, sizeof(code));
        assert(code == 8);

        uint8_t last;
        char box_name[32];

        read_from_channel(fsession, &last, sizeof(last));
        read_from_channel(fsession, box_name, sizeof(box_name));

        if (box_name[0] == '\0') {
            fprintf(stdout, "NO BOXES FOUND\n");
        } else {
            uint64_t box_size;
            uint64_t n_publishers;
            uint64_t n_subscribers;

            while (1) {
                read_from_channel(fsession, &box_size, sizeof(box_size));
                read_from_channel(fsession, &n_publishers,
                                  sizeof(n_publishers));
                read_from_channel(fsession, &n_subscribers,
                                  sizeof(n_subscribers));

                // store box info somewhere, sort alphabetically and print

                if (last)
                    break;

                read_from_channel(fsession, &last, sizeof(last));
                read_from_channel(fsession, box_name, sizeof(box_name));
            }
        }

        close_channel(fsession);
    }

    // delete channel
    delete_channel(argv[2]);
}
