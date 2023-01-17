#include "../utils/logging.h"
#include "../utils/pipe.h"
#include "../utils/protocol.h"
#include "../utils/thread.h"
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define __USE_POSIX199309 1

static int n_messages = 0;
static char *sub_pipe_name;

mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
cond_t cond = PTHREAD_COND_INITIALIZER;

void sigint_handler(int sig) {
    (void)sig;
    pipe_delete(sub_pipe_name);
    if (write(STDOUT_FILENO, &n_messages, sizeof(n_messages)) == -1)
        _exit(EXIT_FAILURE);
    _exit(EXIT_SUCCESS);
}

void update_sigint() {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigact.sa_flags = SA_RESTART;
    sigact.sa_handler = &sigint_handler;

    sigaction(SIGINT, &sigact, NULL);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return EXIT_SUCCESS;
    }

    pipe_create(argv[2], 0640);
    update_sigint();

    {
        LOG("Sending registration request...");
        char buffer[2048];

        serialize_message(buffer, OP_REGISTER_SUB, argv[2], argv[3]);

        pipe_owrite(argv[1], buffer, sizeof(buffer));
    }

    // stores the pipe name
    sub_pipe_name = argv[2];

    {
        int fd = pipe_open(argv[2], O_RDONLY);

        uint8_t code = OP_MSG_SER_TO_SUB;
        char message[1024];
        char buffer[2048];

        if (!pipe_read(fd, buffer, sizeof(buffer))) {
            LOG("Subscriber was rejected by the server.");
            pipe_close(fd);
            pipe_delete(argv[2]);
            return EXIT_SUCCESS;
        }

        LOG("Ready to receive messages.");

        while (1) {
            deserialize_message(buffer, code, message);
            fprintf(stdout, "%s\n", message);

            n_messages++;

            if (!pipe_read(fd, buffer, sizeof(buffer))) {
                break;
            }
        }

        pipe_close(fd);
    }

    pipe_delete(argv[2]);
    return EXIT_SUCCESS;
}
