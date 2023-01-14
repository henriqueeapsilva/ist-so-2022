#include "../utils/channel.h"
#include "../utils/protocol.h"
#include "../utils/box.h"
#include <assert.h>
#include <fcntl.h>
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
        return (argc >= 5) * OP_CREATE_BOX;
    }

    if (!strcmp(argv[3], "remove")) {
        return (argc >= 5) * OP_REMOVE_BOX;
    }

    if (!strcmp(argv[3], "list")) {
        return OP_LIST_BOXES;
    }

    return 0;
}

int main(int argc, char **argv) {
    uint8_t code = eval_request(argc, argv);

    if (!code) {
        print_usage();
        return EXIT_SUCCESS;
    }

    char buffer[2048];

    channel_create(argv[2], 0640);

    { // send request

        if (code == OP_LIST_BOXES) { /* list boxes request */
            serialize_message(buffer, code, argv[2]);
        } else { /* create/remove box request */
            serialize_message(buffer, code, argv[2], argv[4]);
        }

        int fd = channel_open(argv[1], 0640);
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
    }

    { // receive response
        int fd = channel_open(argv[2], O_RDONLY);
        channel_read(fd, buffer, sizeof(buffer));
        channel_close(fd);

        // assumes: response code = request code + 1
        assert(deserialize_code(buffer) == ++code);

        if (code == OP_LIST_BOXES_RET) {
            uint8_t last;
            Box box;

            assert(deserialize_code(buffer) == code);

            deserialize_message(buffer, code, &last, box.name, &box.size,
                                 &box.n_pubs, &box.n_subs);

            if (!*(box.name)) {
                fprintf(stdout, "NO BOXES FOUND\n");
            } else {
                while (!last) {
                    /* TODO: Store boxes somewhere, sort alphabetically and only
                     * then print. */
                    assert(deserialize_code(buffer) == code);
                    deserialize_message(buffer, code, &last, box.name, &box.size,
                                &box.n_pubs, &box.n_subs);
                }
            }
        } else {
            int32_t retcode;
            char errmessage[1024];

            assert(deserialize_code(buffer) == code);
            deserialize_message(buffer, code, &retcode, errmessage);

            if (retcode == -1) {
                fprintf(stdout, "ERROR %s\n", errmessage);
            } else {
                fprintf(stdout, "OK\n");
            }
        }
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}
