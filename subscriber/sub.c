#include "../utils/channel.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>

#define LENGTH 1024 // the length of a message

void sigint_handler(void) {
    
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    // create channel

    create_channel(argv[2], 0640);

    // send registration request

    send_quick_message(argv[1], 2, argv[2], argv[3]);

    // read messages

    int fd = open_channel(argv[2], O_RDONLY);

    uint8_t code;
    char msg[LENGTH];

    while (1) {
        if (!(code = receive_code(fd))) {
            close_channel(fd);
            // wait (block) for some publisher to appear
            fd = open_channel(argv[2], O_RDONLY);
            continue;
        }

        printf("%s\n", msg);
    }

    close_channel(fd);

    // delete channel

    delete_channel(argv[2]);

    return EXIT_SUCCESS;
}
