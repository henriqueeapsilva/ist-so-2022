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

    /* create channel */
    create_channel(argv[2], 0640);

    /* send registration request */
    send_quick_message(argv[1], 2, argv[2], argv[3]);

    /* read messages */
    int fd = open_channel(argv[2], O_RDONLY);

    uint8_t code;
    char buffer[1024];

    while (code = read_code(fd)) {
        read_content(fd, code, buffer);
        puts(buffer);
    }

    close_channel(argv[2]);
    delete_channel(fd);

    return EXIT_SUCCESS;
}
