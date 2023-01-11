#include "../utils/channel.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>

void sigint_handler(void) {
    /* TODO: Handle SIGINT. */
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    channel_create(argv[2], 0640);

    { // Send registration request.
        int fd = channel_open(argv[1], 0640);
        channel_write(fd, REGISTER_SUB, argv[2], argv[3]);
        channel_close(fd);
    }

    { // Receive messages.
        int fd = channel_open(argv[2], O_RDONLY);

        uint8_t code;
        char message[1024];

        while ((code = channel_read_code(fd))) {
            channel_read_content(fd, code, message);
            puts(message);
        }

        channel_close(fd);
    }
    
    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}
