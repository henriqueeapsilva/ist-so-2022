#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

// Helper function to send messages
// Retries to send whatever was not sent in the beginning
void fifo_send_msg(int fd, char const *str) {
    size_t len = strlen(str);
    size_t written = 0;

    while (written < len) {
        written += fifo_write(fd, str + written, len - written);
    }
}

void fifo_make(char *name, mode_t mode) {
    if (mkfifo(name, mode) == -1) {
        fprintf(stderr, "Could not create fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int fifo_open(char *name, int flags) {
    int fhandler = open(name, flags);
    
    if (fhandler == -1) {
        fprintf(stderr, "Could not open fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    return fhandler;
}

ssize_t fifo_write(int fd, const void *buffer, size_t count) {
    ssize_t ret = write(fd, buffer, count);

    if (ret < 0) {
        fprintf(stderr, "Could not write to fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    return ret;
}

ssize_t fifo_read(int fd, const void *buffer, size_t count) {
    ssize_t ret = read(fd, buffer, count);

    if (ret < 0) {
        fprintf(stderr, "Could not read from fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    return ret;
}

void fifo_close(int fhandler) {
    if (close(fhandler) == -1) {
        fprintf(stderr, "Could not close fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void fifo_unlink(char *name) {
    if ((unlink(name) != 0) && (errno != ENOENT)) {
        fprintf(stderr, "Could not unlink fifo (%s): %s\n", name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}
