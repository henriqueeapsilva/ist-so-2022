#include "../utils/channel.h"
#include "../utils/logging.h"
#include "../utils/protocol.h"
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>

void sigint_handler(void) { /* TODO: Handle SIGINT. */ }

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    channel_create(argv[2], 0640);

    { // Send registration request.
        char buffer[2048];

        serialize_message(buffer, OP_REGISTER_SUB, argv[2], argv[3]);

        int fd = channel_open(argv[1], 0640);
        channel_write(fd, buffer, 2048);
        channel_close(fd);
    }

    { // Receive messages.
        int fd = channel_open(argv[2], O_RDONLY);

        uint8_t code = OP_MSG_SER_TO_SUB;
        char message[1024];
        char buffer[2048];

        while (1) {
            channel_read(fd, buffer, sizeof(buffer));

            assert(deserialize_code(buffer) == code);

            deserialize_message(buffer, code, message);
            puts(message);
        }

        channel_close(fd);
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}
