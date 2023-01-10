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

    // create channel

    create_channel(argv[2], 0640);

    // send request

    send_quick_message(argv[1], 1, argv[2], argv[3]);

    // publish messages

    int fd = open_channel(argv[2], O_WRONLY);

    char msg[LENGTH];

    while (scan_message(msg)) {
        send_message(fd, 9, msg);
    }

    close_channel(fd);

    // delete channel

    delete_channel(argv[2]);

    return EXIT_SUCCESS;
}
