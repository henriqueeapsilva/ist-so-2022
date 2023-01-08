#include "../utils/channel.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

#define LENGTH 1024 // the length of a message

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    // create channel

    create_channel(argv[2], 0640);

    // send registration request

    {
        int fd = open_channel(argv[1], 0640);

        uint8_t code = 2;
        memwrite_to_channel(fd, &code);
        strwrite_to_channel(fd, argv[2], 256);
        strwrite_to_channel(fd, argv[3], 32);

        close_channel(fd);
    }

    // read messages

    {
        int fsession = open_channel(argv[2], O_RDONLY);

        char msg[LENGTH];

        while (read_from_channel(fsession, msg, LENGTH) > 0) {
            printf("%s\n", msg);
        }

        close_channel(fsession);
    }

    delete_channel(argv[2]);

    return EXIT_SUCCESS;
}
