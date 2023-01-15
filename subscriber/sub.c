#include "../utils/channel.h"
#include "../utils/logging.h"
#include "../utils/protocol.h"
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "../utils/thread.h"

#define __USE_POSIX199309 1

static int fd;
static int readed_message = 0;
static char* pipe_name;

mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
cond_t cond = PTHREAD_COND_INITIALIZER;


void sigint_handler(int sig) {
    DEBUG("Terminating session: closing channel.");
    if(sig != SIGINT) _exit(EXIT_FAILURE);
    if(signal(SIGINT, sigint_handler) == SIG_ERR){
        channel_delete(pipe_name);
        LOG("Session terminated.");
        if(write(STDOUT_FILENO, &readed_message, sizeof(readed_message)) < 0) _exit(EXIT_FAILURE);
        _exit(EXIT_SUCCESS);
    }

    channel_delete(pipe_name);
    LOG("Session terminated.");
    if(write(STDOUT_FILENO, &readed_message, sizeof(readed_message)) < 0) _exit(EXIT_FAILURE);
    _exit(EXIT_SUCCESS);
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

    {
        LOG("Sending registration request...");
        char buffer[2048];

        serialize_message(buffer, OP_REGISTER_SUB, argv[2], argv[3]);

        fd = channel_open(argv[1], O_WRONLY);
        channel_write(fd, buffer, sizeof(buffer));
        channel_close(fd);
        LOG("Registration request sent.");
    }

    // stores the pipe name
    pipe_name = argv[2];

    {
        fd = channel_open(argv[2], O_RDONLY);

        uint8_t code = OP_MSG_SER_TO_SUB;
        char message[1024];
        char buffer[2048];

        if (!channel_read(fd, buffer, sizeof(buffer))) {
            LOG("Subscriber was rejected by the server.");
            channel_close(fd);
            channel_delete(argv[2]);
            return EXIT_SUCCESS;
        }

        LOG("Ready to receive messages.");

        while (1) {
            assert(deserialize_code(buffer) == code);

            deserialize_message(buffer, code, message);
            fprintf(stdout, "%s\n", message);

            readed_message++;

            DEBUG("Messages counter: %d", readed_message);

            channel_read(fd, buffer, sizeof(buffer));
        }
    }

    return EXIT_SUCCESS;
}
