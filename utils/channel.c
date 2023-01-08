#include "channel.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
/* Channel Handler */

void create_channel(const char *name, mode_t mode) {
    if (mkfifo(name, mode) == -1) {
        fprintf(stderr, "Could not create fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void delete_channel(const char *name) {
    if ((unlink(name) != 0) && (errno != ENOENT)) {
        fprintf(stderr, "Could not unlink fifo (%s): %s\n", name,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int open_channel(char *name, int flags) {
    int fd = open(name, flags);

    if (fd == -1) {
        fprintf(stderr, "Could not open fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fd;
}

void close_channel(int fd) {
    if (close(fd) == -1) {
        fprintf(stderr, "Could not close fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

size_t write_to_channel(int fd, const void *buffer, size_t len) {
    ssize_t ret = write(fd, buffer, len);

    if (ret < 0) {
        fprintf(stderr, "Could not write to fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return (size_t) ret;
}

size_t read_from_channel(int fd, void *buffer, size_t len) {
    ssize_t ret = read(fd, buffer, len);

    if (ret < 0) {
        fprintf(stderr, "Could not read from fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return (size_t) ret;
}

void fwrite_to_channel(int fd, const void *buffer, size_t len) {
    size_t written = 0;

    while (written < len) {
        written += write_to_channel(fd, (buffer + written), (len - written));
    }
}

void memwrite_to_channel(int fd, void *ptr) {
    fwrite_to_channel(fd, ptr, sizeof(ptr));
}

void strwrite_to_channel(int fd, char *string, size_t len) {
    char buffer[len];
    strncpy(buffer, string, len);
    buffer[len - 1] = '\0';
    fwrite_to_channel(fd, buffer, len);
}
