#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

// Helper function to send messages
// Retries to send whatever was not sent in the beginning
void send_msg(int tx, char const *str, int fill) {
    size_t len = strlen(str);
    size_t written = 0;

    while (written < len) {
        ssize_t ret = write(tx, str + written, len - written);
        if (ret < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        written += ret;
    }
}

void fifo_make(char *name, int mode) {

}

int fifo_open(char *name, int mode) {
    if (open(name, mode) == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void fifo_write_msg(int fhandler) {

}

void fifo_read_msg(int fhandler) {

}

void fifo_close(int fhandler) {
    if (close(fhandler) == -1) {
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void fifo_unlink(char *name) {
    if (unlink(name) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}
