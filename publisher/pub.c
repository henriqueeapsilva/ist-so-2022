#include "../utils/channel.h"
#include "../utils/protocol.h"
#include "../utils/logging.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int scan_message(char *msg) {
    char *end = msg+sizeof(msg);

    while (msg < (end-1)) {
        *msg = (char) getchar();

        if (*msg == '\n') {
            break;
        }

        if (*msg == EOF) {
            return -1;
        }
    }

    while (msg < end) {
        *(msg++) = 0;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigact.sa_flags = SA_RESTART;
    sigact.sa_handler = SIG_IGN;

    channel_create(argv[2], 0640);

    {
        char buffer[2048];

        serialize_message(buffer, OP_REGISTER_PUB, argv[2], argv[3]);

        LOG("Sending registration request...");
        int fd = channel_open(argv[1], O_WRONLY);
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        LOG("Registration request sent.");
    }

    {
        int fd = channel_open(argv[2], O_WRONLY);

        uint8_t code = OP_MSG_PUB_TO_SER;
        char message[1024];
        char buffer[2048];

        LOG("Ready to publish messages.");

        // ignores the SIGPIPE
        sigaction(SIGPIPE, &sigact, NULL);

        // verification: channel ready?
        if(channel_write(fd, buffer, sizeof(buffer)) == -1){
            channel_close(fd);
            channel_delete(argv[2]);
            LOG("session failed: channel closed.")
            return EXIT_SUCCESS;
        }

        while (scan_message(message) != -1) {
            serialize_message(buffer, code, message);
            
            channel_write(fd, buffer, sizeof(buffer));
        }

        channel_close(fd);
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}