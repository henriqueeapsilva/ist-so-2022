#include "../utils/channel.h"

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

    // create channel

    create_channel(argv[2], 0640);

    // send request

    {
        int fregister = open_channel(argv[1], O_WRONLY);

        uint8_t code = 1;
        memwrite_to_channel(fregister, &code);
        strwrite_to_channel(fregister, argv[2], 256);
        strwrite_to_channel(fregister, argv[3], 32);

        close_channel(fregister);
    }

    // publish messages

    {
        int fsession = open_channel(argv[2], O_WRONLY);

        char msg[LENGTH];
        int i = 0;

        msg[i] = getchar();

        while (msg[i] != EOF) {
            if (msg[i] == '\n') {
                memset(msg + i, '\0', (size_t) (LENGTH - i));
                memwrite_to_channel(fsession, msg);
                i = 0;
            } else if (i == (LENGTH - 1)) {
                do {
                    msg[i] = getchar();
                } while ((msg[i] != EOF) && (msg[i] != '\n'));
                i = 0;
                continue;
            }

            msg[i++] = getchar();
        }

        close_channel(fsession);
    }

    // delete channel

    delete_channel(argv[2]);

    return EXIT_SUCCESS;
}
