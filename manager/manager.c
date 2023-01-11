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

static uint8_t eval_request(int argc, char **argv) {
    if (argc < 4)
        return 0;

    if (!strcmp(argv[3], "create")) {
        return (argc >= 5) * CREATE_BOX;
    }

    if (!strcmp(argv[3], "remove")) {
        return (argc >= 5) * REMOVE_BOX;
    }

    if (!strcmp(argv[3], "list")) {
        return LIST_BOXES;
    }

    return 0;
}

int main(int argc, char **argv) {
    uint8_t code = eval_request(argc, argv);

    if (!code) {
        print_usage();
        return EXIT_SUCCESS;
    }

    channel_create(argv[2], 0640);


    { // send request
        int fd = channel_open(argv[1], 0640);

        if (code == LIST_BOXES) { /* list boxes request */
            channel_write(fd, code, argv[2]);
        } else { /* create/remove box request */
            channel_write(fd, code, argv[2], argv[4]);
        }

        channel_close(fd);
    }

    { // receive response
        int fd = channel_open(argv[2], O_RDONLY);

        // assumes: response code = request code + 1
        assert(channel_read_code(fd) == ++code);

        if (code == LIST_BOXES_RET) {
            uint8_t last;
            char box_name[32];
            uint64_t box_size;
            uint64_t n_publishers;
            uint64_t n_subscribers;

            assert(channel_read_code(fd) == code);
            channel_read_content(fd, code, &last, box_name, &box_size, &n_publishers, &n_subscribers);

            if (!*box_name) {
                fprintf(stdout, "NO BOXES FOUND\n");
            } else {
                while (!last) {
                    /* TODO: Store boxes somewhere, sort alphabetically and only then print. */
                    assert(channel_read_code(fd) == code);
                    channel_read_content(fd, code, &last, box_name, &box_size, &n_publishers, &n_subscribers);
                }
            }
        } else {
            int32_t errcode;
            char errmessage[1024];

            assert(channel_read_code(fd) == code);
            channel_read_content(fd, code, &errcode, errmessage);

            if (errcode) {
                fprintf(stdout, "ERROR %s\n", errmessage);
            } else {
                fprintf(stdout, "OK\n");
            }
        }

        channel_close(fd);
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}
