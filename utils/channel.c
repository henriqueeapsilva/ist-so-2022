#include "channel.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>

void channel_create(const char *name, mode_t mode) {
    if (mkfifo(name, mode) == -1) {
        fprintf(stderr, "Could not create fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void channel_delete(const char *name) {
    if ((unlink(name) == -1) && (errno != ENOENT)) {
        fprintf(stderr, "Could not unlink fifo (%s): %s\n", name,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int channel_open(const char *name, int flags) {
    int fd = open(name, flags);

    if (fd == -1) {
        fprintf(stderr, "Could not open fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fd;
}

void channel_close(int fd) {
    if (close(fd) == -1) {
        fprintf(stderr, "Could not close fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

ssize_t channel_write(int fd, void *buffer, size_t len) {
    ssize_t remaining = (ssize_t) len;
    ssize_t ret;

    while ((ret = write(fd, buffer, remaining)) && remaining) {
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        buffer += ret;
        remaining -= ret;
    }

    return (len - remaining);
}

ssize_t channel_read(int fd, void *buffer, size_t len) {
    ssize_t remaining = (ssize_t) len;
    ssize_t ret;

    while ((ret = read(fd, buffer, len)) && remaining) {
        if (ret == -1) {
            if (errno != EINTR) {
                return -1;
            }

            continue;
        }

        buffer += ret;
        remaining -= ret;
    }

    return remaining;
}
