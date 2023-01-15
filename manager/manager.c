#include "../utils/channel.h"
#include "../utils/protocol.h"
#include "../utils/box.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../utils/logging.h"

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

static int boxcmp(const void *e1, const void *e2) {
    Box *box1 = (Box*) e1;
    Box *box2 = (Box*) e2;

    return strcmp(box1->name, box2->name);
}

int main(int argc, char **argv) {
    uint8_t code = eval_request(argc, argv);

    if (!code) {
        print_usage();
        return EXIT_SUCCESS;
    }

    char buffer[2048];

    channel_create(argv[2], 0640);

    {
        if (code == OP_LIST_BOXES) { /* list boxes request */
            serialize_message(buffer, code, argv[2]);
        } else { /* create/remove box request */
            serialize_message(buffer, code, argv[2], argv[4]);
        }

        LOG("Sending registration request (code: %d)...", code);
        int fd = channel_open(argv[1], O_WRONLY);
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        LOG("Registration request sent.");
    }

    {
        int fd = channel_open(argv[2], O_RDONLY);
        LOG("Reading response...");
        channel_read(fd, buffer, sizeof(buffer));
        LOG("Response received.");
        channel_close(fd);
        
        DEBUG("%d %d", deserialize_code(buffer), code);

        // assumes: response code = request code + 1
        assert(deserialize_code(buffer) == ++code);

        if (code == OP_LIST_BOXES_RET) {
            uint8_t last;
            Box box;

            deserialize_message(buffer, code, &last, box.name, &box.size,
                                 &box.n_pubs, &box.n_subs);

            if (!*(box.name)) {
                fprintf(stdout, "NO BOXES FOUND\n");
            } else {
                Box boxes[64];
                size_t count = 0;
                boxes[count++] = box;

                while (!last) {
                    box = boxes[count++];
                    assert(deserialize_code(buffer) == code);
                    deserialize_message(buffer, code, &last, box.name, &box.size,
                                &box.n_pubs, &box.n_subs);
                }

                qsort(boxes, count, sizeof(Box), boxcmp);

                for (int i = 0; i < count; i++) {
                    box = boxes[i];
                    fprintf(stdout, "%s %zu %zu %zu\n", box.name, box.size, box.n_pubs, box.n_subs);
                }
            }
        } else {
            int32_t retcode;
            char errmessage[1024];

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
