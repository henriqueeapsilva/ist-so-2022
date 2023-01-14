#include "../utils/channel.h"
#include "../utils/protocol.h"
#include "../utils/logging.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    channel_create(argv[2], 0640);

    { // Send registation request.
        char buffer[2048];

        serialize_message(buffer, OP_REGISTER_PUB, argv[2], argv[3]);

        DEBUG("Sending registration request...");
        int fd = channel_open(argv[1], O_WRONLY);
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        DEBUG("Registration request sent.");
    }

    { // Publish messages.
        DEBUG("Starting session: opening channel.");
        int fd = channel_open(argv[2], O_WRONLY);
        DEBUG("Session started.");

        uint8_t code = OP_MSG_PUB_TO_SER;
        char message[1024];
        char buffer[2048];
        int i = 0;
        int c;

        // verifies if nothing went wrong
        serialize_message(buffer, code, "b");
        if (channel_write(fd, buffer, sizeof(buffer)) == -1) {
            channel_close(fd);
            channel_delete(argv[3]);
            DEBUG("Session terminated.");
            return EXIT_SUCCESS;
        }

        while ((c = getchar()) != EOF) {
            message[i] = (char) c;

            if (c != '\n') {
                if (++i < sizeof(buffer) - 1) {
                    continue;
                }

                while (((c = getchar()) != EOF) && (c != '\n'));

                if (c == EOF) {
                    break;
                }
            }

            message[i] = 0;

            serialize_message(buffer, code, message);

            if (channel_write(fd, buffer, sizeof(buffer)) == -1) {
                channel_close(fd);
                channel_delete(argv[3]);
                return EXIT_SUCCESS;
            }

            i = 0;
        }

        DEBUG("Terminating session: closing channel.");
        channel_close(fd);
        DEBUG("Session terminated.");
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}