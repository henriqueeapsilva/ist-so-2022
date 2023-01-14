#include "../utils/channel.h"
#include "../utils/protocol.h"

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
        printf("111");
        int fd = channel_open(argv[1], 0640);
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
    }

    { // Publish messages.
        int fd = channel_open(argv[2], O_WRONLY);

        uint8_t code = OP_MSG_PUB_TO_SER;
        char message[1024];
        char buffer[2048];
        int i = 0;
        int c;

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
            if(channel_write(fd, buffer, sizeof(buffer)) == -1){
                channel_close(fd);
                channel_delete(argv[3]);
                return EXIT_SUCCESS;
            }
            i = 0;
        }

        channel_close(fd);
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}
