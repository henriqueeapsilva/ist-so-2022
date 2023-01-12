#include "../utils/channel.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_LENGTH 1024 // the length of a message

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    channel_create(argv[2], 0640);

    { // Send registation request.
        int fd = channel_open(argv[1], 0640);

        channel_write(fd, OP_REGISTER_PUB, argv[2], argv[3]);

        channel_close(fd);
    }

    { // Publish messages.
        int fd = channel_open(argv[2], O_WRONLY);

        char buf[BUF_LENGTH];
        int i = 0;

        while ((buf[i] = getchar()) != EOF) {
            if (buf[i] != '\n') {
                if (++i < BUF_LENGTH-1) {
                    continue;
                }

                while (((buf[i] = getchar()) != EOF) && (buf[i] != '\n'));

                if (buf[i] == EOF) {
                    break;
                }
            }

            buf[i] = 0;
            channel_write(fd, OP_MSG_PUB_TO_SER, buf);
            i = 0;
        }

        channel_close(fd);
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}
