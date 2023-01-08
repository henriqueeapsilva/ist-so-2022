#include "../utils/channel.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

bool eval_request(int argc, char **argv);
void create_box(char *reg_pipe_path, char *pipe_path, char *box_name);
void remove_box(char *reg_pipe_path, char *pipe_path, char *box_name);
void list(char *reg_pipe_path, char *pipe_path);

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

        create_box(argv[1], argv[2], argv[4]);
        return true;
    }

    if (!strcmp(argv[3], "remove")) {
        if (argc < 5)
            return false;

        remove_box(argv[1], argv[2], argv[4]);
        return true;
    }

    if (!strcmp(argv[3], "list")) {
        list(argv[1], argv[2]);
        return true;
    }

    return false;
}

void update_box(uint8_t request, char *reg_pipe_path, char *pipe_path, char *box_name) {
    // create channel

    create_channel(pipe_path, 0640);

    // send request

    {
        int fregistry = open_channel(reg_pipe_path, O_WRONLY);

        memwrite_to_channel(fregistry, (uint8_t *)&request);
        strwrite_to_channel(fregistry, pipe_path, 256);
        strwrite_to_channel(fregistry, box_name, 32);

        close_channel(fregistry);
    }

    // receive response

    {
        int fsession = open_channel(pipe_path, O_WRONLY);

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

    delete_channel(pipe_path);
}

void create_box(char *reg_pipe_path, char *pipe_path, char *box_name) {
    update_box(3, reg_pipe_path, pipe_path, box_name);
}

void remove_box(char *reg_pipe_path, char *pipe_path, char *box_name) {
    update_box(5, reg_pipe_path, pipe_path, box_name);
}

void list(char *reg_pipe_path, char *pipe_path) {
    // create channel

    create_channel(pipe_path, 0640);

    // send request

    {
        int fregistry = open_channel(reg_pipe_path, O_WRONLY);

        uint8_t request = 7;
        memwrite_to_channel(fregistry, (uint8_t *)&request);
        strwrite_to_channel(fregistry, pipe_path, 256);

        close_channel(fregistry);
    }

    // receive response

    {
        int fsession = open_channel(pipe_path, O_WRONLY);

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
    delete_channel(pipe_path);
}
