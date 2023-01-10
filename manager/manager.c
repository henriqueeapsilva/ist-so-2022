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

static int eval_request(int argc, char **argv) {
    if (argc < 4)
        return 0;

    if (!strcmp(argv[3], "create")) {
        return (argc >= 5) * 3;
    }

    if (!strcmp(argv[3], "remove")) {
        return (argc >= 5) * 5;
    }

    if (!strcmp(argv[3], "list")) {
        return 7;
    }

    return 0;
}

int main(int argc, char **argv) {
    uint8_t code = eval_request(argc, argv);

    if (!code) {
        print_usage();
        return EXIT_SUCCESS;
    }

    create_channel(argv[2], 0640);

    if (code == 7) { /* list boxes request */
        send_quick_message(argv[1], code, argv[2]);
    } else { /* create/remove box request */
        send_quick_message(argv[1], code, argv[2], argv[4]);
    }

    int fd = open_channel(argv[2], O_RDONLY);
    assert(receive_code(fd) == ++code);

    if (code == 8) { /* list boxes response */
        int32_t errcode;
        char errmessage[1024];

        receive_content(fd, code, &errcode, errmessage);

        if (errcode) {
            fprintf(stdout, "ERROR %s\n", errmessage);
        } else {
            fprintf(stdout, "OK\n");
        }
    } else { /* create/remove box response */
        uint8_t code = 7;
        uint8_t last;
        char box_name[32];
        uint64_t box_size;
        uint64_t n_publishers;
        uint64_t n_subscribers;

        receive_content(fd, code, &last, box_name, &box_size, &n_publishers, &n_subscribers);

        if (!*box_name) {
            fprintf(stdout, "NO BOXES FOUND\n");
        } else {
            while (!last) {
                assert(receive_code(fd) == code);
                receive_content(fd, code, &last, box_name, &box_size, &n_publishers, &n_subscribers);
            }
        }
    }

    close_channel(fd);
    delete_channel(argv[2]);

    return EXIT_SUCCESS;
}
