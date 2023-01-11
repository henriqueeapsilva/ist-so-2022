#include "../utils/channel.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LENGTH 1024 // the length of a message

int scan_message(char *msg) {
    int i = 0;
    
    while ((msg[i++] = (char) getchar()) != EOF) {
        if (msg[i] == '\n') {
            memset(msg+i, '\0', (size_t) (LENGTH-i));
            return 1;
        }

        if (i == (LENGTH-1)) {
            do {
                msg[i] = (char) getchar();
            } while ((msg[i] != EOF) && (msg[i] != '\n'));

            if (msg[i] == EOF) return 0;

            msg[i] = '\0';
            return 1;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    channel_create(argv[2], 0640);

    { // Send registation request.
        int fd = channel_open(argv[1], 0640);

        channel_write(fd, REGISTER_PUB, argv[2], argv[3]);

        channel_close(fd);
    }

    { // Publish messages.
        int fd = channel_open(argv[2], O_WRONLY);

        char msg[LENGTH];

        while (scan_message(msg)) {
            channel_write(fd, MSG_PUB_TO_SER, msg);
        }

        channel_close(fd);
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}
