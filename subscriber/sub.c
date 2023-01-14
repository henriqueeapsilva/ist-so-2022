#include "../utils/channel.h"
#include "../utils/logging.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define __USE_POSIX199309 1

static int fd;
static int readed_message = 0;

void sigint_handler(int sig) {
    if(sig == SIGINT){
        if(signal(SIGINT, sigint_handler) == SIG_ERR){
            channel_close(fd);
            if(write(STDOUT_FILENO, &readed_message, sizeof(readed_message)) < 0) _exit(EXIT_FAILURE);
            _exit(EXIT_SUCCESS);
        }
        channel_close(fd);
        if(write(STDOUT_FILENO, &readed_message, sizeof(readed_message)) < 0) _exit(EXIT_FAILURE);
        _exit(EXIT_SUCCESS);
    }
    _exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigact.sa_flags = SA_RESTART;
    sigact.sa_handler = &sigint_handler;

    sigaction(SIGINT, &sigact, NULL);

    channel_create(argv[2], 0640);

    { // Send registration request.
        fd = channel_open(argv[1], 0640);
        channel_write(fd, OP_REGISTER_SUB, argv[2], argv[3]);
        channel_close(fd);
    }

    { // Receive messages.
        fd = channel_open(argv[2], O_RDONLY);

        uint8_t code;
        char message[1024];

        while ((code = channel_read_code(fd))) {
            readed_message++;
            channel_read_content(fd, code, message);
            puts(message);
        }

        channel_close(fd);
    }

    channel_delete(argv[2]);

    return EXIT_SUCCESS;
}
