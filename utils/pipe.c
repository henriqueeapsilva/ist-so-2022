#include "pipe.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void pipe_create(const char *name, mode_t mode) {
    if (mkfifo(name, mode) == -1) {
        fprintf(stderr, "Could not create fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void pipe_delete(const char *name) {
    if ((unlink(name) == -1) && (errno != ENOENT)) {
        fprintf(stderr, "Could not unlink fifo (%s): %s\n", name,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int pipe_open(const char *name, int flags) {
    int fd = open(name, flags);

    if (fd == -1) {
        fprintf(stderr, "Could not open fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fd;
}

void pipe_close(int fd) {
    if (close(fd) == -1) {
        fprintf(stderr, "Could not close fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

ssize_t pipe_write(int fd, void *buffer, size_t len) {
    size_t remaining = len;
    ssize_t ret;

    while ((ret = write(fd, buffer, remaining)) && remaining) {
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }

            if (errno == EPIPE) {
                return -1;
            }

            fprintf(stderr, "Could not write to fifo: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        buffer += ret;
        remaining -= (size_t)ret;
    }

    return (ssize_t)(len - remaining);
}

ssize_t pipe_read(int fd, void *buffer, size_t len) {
    size_t remaining = len;
    ssize_t ret;

    while ((ret = read(fd, buffer, remaining)) && remaining) {
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }

            fprintf(stderr, "Could not read from fifo: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        buffer += ret;
        remaining -= (size_t)ret;
    }

    return (ssize_t)(len - remaining);
}
