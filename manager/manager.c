#include "utils/channel.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static enum resquest_t { CREATE=3, REMOVE=5, LIST=7, UNDEF=-1 };

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

bool eval_request(int argc, char **argv);
void create_box(int argc, char **argv);
void remove_box(int argc, char **argv);
void list(int argc, char **argv);

int main(int argc, char **argv) {
    eval_request(argc, argv);
    return EXIT_SUCCESS;
}

bool eval_request(int argc, char **argv) {
    if (argc < 4) return false;

    if (!strcmp(argv[3], "create")) {
        if (argc < 5) return false;

        create_box(argc, argv);
        return true;
    }

    if (!strcmp(argv[3], "remove")) {
        if (argc < 5) return false;

        remove_box(argc, argv);
        return true;
    }

    if (!strcmp(argv[3], "list")) {
        list(argc, argv);
        return true;
    }
}

void update_box(int argc, char **argv, uint8_t request) {
    // load channels

    create_channel(argv[2], 0640);

    int fregistry = open_channel(argv[1], O_WRONLY);
    int fsession = open_channel(argv[2], O_WRONLY);

    // send request

    memwrite_to_channel(fregistry, (uint8_t*) &request);
    strwrite_to_channel(fregistry, argv[2], 256);
    strwrite_to_channel(fregistry, argv[4], 32);

    // receive response

    uint8_t code;
    read_from_channel(fsession, &code, sizeof(code));
    assert(code == (request+1));

    int32_t failed;
    read_from_channel(fsession, &failed, sizeof(failed));

    if (failed) {
        char message[1024];
        read_from_channel(fsession, message, sizeof(message));
        fprintf(stdout, "ERROR %s\n", message);
    } else {
        fprintf(stdout, "OK\n");
    }

    // unload channels

    close_channel(fregistry);
    close_channel(fsession);
    delete_channel(argv[2]);
}

void create_box(int argc, char **argv) {
    update_box(argc, argv, 3);
}

void remove_box(int argc, char **argv) {
    update_box(argc, argv, 5);
}

void list(int argc, char **argv) {
    // load channels

    create_channel(argv[2], 0640);

    int fregistry = open_channel(argv[1], O_WRONLY);
    int fsession = open_channel(argv[2], O_WRONLY);

    // send request

    uint8_t request = 7;
    memwrite_to_channel(fregistry, (uint8_t*) &request);
    strwrite_to_channel(fregistry, argv[2], 256);

    // receive response

    uint8_t code;
    read_from_channel(fsession, &code, sizeof(code));
    assert(code == 8);

    uint8_t last;
    char box_name[32];

    read_from_channel(fsession, &last, sizeof(last));
    read_from_channel(fsession, box_name, sizeof(box_name));

    if (last && (box_name[0] == '\0')) {
        fprintf(stdout, "NO BOXES FOUND\n");
    } else {
        uint64_t box_size;
        uint64_t n_publishers;
        uint64_t n_subscribers;

        while (1) {
            read_from_channel(fsession, &box_size, sizeof(box_size));
            read_from_channel(fsession, &n_publishers, sizeof(n_publishers));
            read_from_channel(fsession, &n_subscribers, sizeof(n_subscribers));

            // store box info somewhere, sort alphabetically and print

            if (last) break;

            read_from_channel(fsession, &last, sizeof(last));
            read_from_channel(fsession, box_name, sizeof(box_name));
        }
    }


    // unload channels

    close_channel(fregistry);
    close_channel(fsession);
    delete_channel(argv[2]);
}
