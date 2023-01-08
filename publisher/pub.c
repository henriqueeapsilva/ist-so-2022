#include "utils/channel.h"
#include "utils/logging.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LENGTH 1024 // the length of a message

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    // load channels

    create_channel(argv[2], 0640);

    int fregister = open_channel(argv[1], O_WRONLY);
    int fsession = open_channel(argv[2], O_WRONLY);

    // send request

    uint8_t code = 1;
    memwrite_to_channel(fregister, &code);
    strwrite_to_channel(fregister, argv[2], 256);
    strwrite_to_channel(fregister, argv[3], 32);

    // publish messages

    char msg[LENGTH];
    char c = getchar();
    int i = 0;

    while (c != EOF) {
        if (c == '\n') {
            memset(msg + i, '\0', LENGTH - i);
            memwrite_to_channel(fsession, msg);
            continue;
        }

        if ((i == LENGTH - 1)) {
            do {
                c = getchar();
            } while ((c != EOF) && (c != '\n'));
            continue;
        }

        c = getchar();
    }

    // unload channels

    close_channel(fregister);
    close_channel(fsession);
    delete_channel(argv[2]);

    return EXIT_SUCCESS;
}
