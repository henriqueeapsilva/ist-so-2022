#include "../utils/channel.h"
#include <fcntl.h>
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

int eval_request(int argc, char **argv);
void update_box(uint8_t code, char *reg_pipe_path, char *pipe_path, char *box_name);
void list(char *reg_pipe_path, char *pipe_path);

int main(int argc, char **argv) {
    if (!eval_request(argc, argv)) {
        print_usage();
    }

    return EXIT_SUCCESS;
}

int eval_request(int argc, char **argv) {
    if (argc < 4)
        return 0;

    if (!strcmp(argv[3], "create")) {
        if (argc < 5)
            return 0;

        update_box(3, argv[1], argv[2], argv[4]);
        return 1;
    }

    if (!strcmp(argv[3], "remove")) {
        if (argc < 5)
            return 0;

        update_box(5, argv[1], argv[2], argv[4]);
        return 1;
    }

    if (!strcmp(argv[3], "list")) {
        list(argv[1], argv[2]);
        return 1;
    }

    return 0;
}

void update_box(uint8_t code, char *reg_pipe_path, char *pipe_path, char *box_name) {
    // create channel

    create_channel(pipe_path, 0640);

    // send request

    send_quick_message(reg_pipe_path, code, pipe_path, box_name);

    // receive response

    int fd = open_channel(pipe_path, O_RDONLY);

    int32_t error_code;
    char error_message[1024];

    assert(receive_code(fd) == ++code);
    receive_content(fd, code, &error_code, error_message);

    if (error_code) {
        fprintf(stdout, "ERROR %s\n", error_message);
    } else {
        fprintf(stdout, "OK\n");
    }

    close_channel(fd);

    // delete channel

    delete_channel(pipe_path);
}

void list(char *reg_pipe_path, char *pipe_path) {
    // create channel

    create_channel(pipe_path, 0640);

    // send request

    send_quick_message(reg_pipe_path, 7, pipe_path);

    // receive response

    int fd = open_channel(pipe_path, O_RDONLY);

    uint8_t code = 7;
    uint8_t last;
    char box_name[32];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;

    assert(receive_code(fd) == code);
    receive_content(fd, code, &last, box_name, &box_size, &n_publishers, &n_subscribers);

    if (!*box_name) {
        fprintf(stdout, "NO BOXES FOUND\n");
    } else {
        while (!last) {
            assert(receive_code(fd) == code);
            receive_content(fd, code, &last, box_name, &box_size, &n_publishers, &n_subscribers);
        }
    }

    close_channel(fd);

    // delete channel

    delete_channel(pipe_path);
}
