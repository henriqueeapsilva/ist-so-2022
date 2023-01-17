#include "../utils/box.h"
#include "../utils/logging.h"
#include "../utils/pipe.h"
#include "../utils/protocol.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

static uint8_t eval_request(int argc, char **argv) {
    LOG("Evaluating request...");

    if (argc < 4) {
        return 0;
    }

    if (!strcmp(argv[3], "create")) {
        return (argc > 4) * OP_CREATE_BOX;
    }

    if (!strcmp(argv[3], "remove")) {
        return (argc > 4) * OP_REMOVE_BOX;
    }

    if (!strcmp(argv[3], "list")) {
        return OP_LIST_BOXES;
    }

    return 0;
}

static int boxcmp(const void *e1, const void *e2) {
    Box *box1 = (Box *)e1;
    Box *box2 = (Box *)e2;

    return strcmp(box1->name, box2->name);
}

int main(int argc, char **argv) {
    uint8_t code = eval_request(argc, argv);

    if (!code) {
        print_usage();
        return EXIT_SUCCESS;
    }

    pipe_create(argv[2], 0640);

    {
        LOG("Sending registration request (code: %d)...", code);

        char buffer[2048];

        if (code == OP_LIST_BOXES) {
            serialize_message(buffer, code, argv[2]);
        } else {
            serialize_message(buffer, code, argv[2], argv[4]);
        }

        pipe_owrite(argv[1], buffer, sizeof(buffer));
    }

    {
        LOG("Reading response...");

        char buffer[2048];

        int fd = pipe_open(argv[2], O_RDONLY);
        pipe_read(fd, buffer, sizeof(buffer));

        assert(deserialize_code(buffer) == ++code);

        if (code == OP_LIST_BOXES_RET) {
            Box boxes[64];
            Box *end = boxes;

            for (uint8_t last = 0; !last; end++) {
                pipe_read(fd, buffer, sizeof(buffer));

                deserialize_message(buffer, code, &last, end->name,
                                    &end->size, &end->n_pubs, &end->n_subs);
            }

            if (!*(end->name)) {
                fprintf(stdout, "NO BOXES FOUND\n");
                return EXIT_SUCCESS;
            }

            qsort(boxes, (size_t) (++end - boxes), sizeof(Box), boxcmp);

            for (Box *box = boxes; (box < end); box++) {
                fprintf(stdout, "%s %zu %zu %zu\n", box->name, box->size,
                        box->n_pubs, box->n_subs);
            }
        } else {
            int32_t errcode;
            char errmessage[1024];

            deserialize_message(buffer, code, &errcode, errmessage);

            if (errcode == -1) {
                fprintf(stdout, "ERROR %s\n", errmessage);
            } else {
                fprintf(stdout, "OK\n");
            }
        }

        pipe_close(fd);
    }

    pipe_delete(argv[2]);
    return EXIT_SUCCESS;
}
