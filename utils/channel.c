#include "channel.h"
#include "box.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>

/* Channel Handler */

void create_channel(const char *name, mode_t mode) {
    if (mkfifo(name, mode) == -1) {
        fprintf(stderr, "Could not create fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void delete_channel(const char *name) {
    if ((unlink(name) == -1) && (errno != ENOENT)) {
        fprintf(stderr, "Could not unlink fifo (%s): %s\n", name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int open_channel(const char *name, int flags) {
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

static size_t write_to_channel(int fd, const void *buffer, size_t len) {
    ssize_t ret;

    while ((ret = write(fd, buffer, len) == -1) && (errno == EINTR));

    if (ret == -1) {
        fprintf(stderr, "Could not write to fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return (size_t) ret;
}

static size_t read_from_channel(int fd, void *buffer, size_t len) {
    ssize_t ret;

    while ((ret = read(fd, buffer, len) == -1) && (errno == EINTR));

    if (ret == -1) {
        fprintf(stderr, "Could not read from fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return (size_t) ret;
}

static void fwrite_to_channel(int fd, const void *buffer, size_t len) {
    size_t written = 0;

    while (written < len) {
        written += write_to_channel(fd, (buffer + written), (len - written));
    }
}

static size_t fread_from_channel(int fd, void *buffer, size_t len) {
    size_t read = 0;

    do {
        read += read_from_channel(fd, (buffer + read), (len - read));
    } while ((read < len) && (read != 0));

    return read;
}

/**
 * Writes a piece of data to the channel.
 */
static void memwrite_to_channel(int fd, void *ptr) {
    fwrite_to_channel(fd, ptr, sizeof(ptr));
}

/**
 * Writes a string to the channel, filling the end with '\0'.
 */
static void strwrite_to_channel(int fd, char *string, size_t len) {
    char buffer[len];
    buffer[len - 1] = '\0';
    fwrite_to_channel(fd, strncpy(buffer, string, len - 1), len);
}

static void vsend_message(int fd, uint8_t code, va_list ap) {
    switch (code) {
        case 1: /* register publisher request */
        case 2: /* register subscriber request */
        case 3: /* create box request */
        case 5: /* remove box request */
            /*
             * 1. code (uint8_t)
             * 2. client_named_pipe_path (char[256])
             * 3. box_name (char[32])
             */
            memwrite_to_channel(fd, &code);
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_CHANNEL_NAME_SIZE);
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_BOX_NAME_SIZE);
            break;
        case 4: /* create box response */
        case 6: /* remove box response */
            /*
             * 1. code (uint8_t)
             * 2. error_code (int32_t)
             * 3. error_message (char[1024])
             */
            memwrite_to_channel(fd, &code);
            memwrite_to_channel(fd, va_arg(ap, int32_t*));
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_MESSAGE_SIZE);
            break;
        case 7: /* list boxes request */
            /*
             * 1. code (uint8_t)
             * 2. client_named_pipe_path (char[256])
             */
            memwrite_to_channel(fd, &code);
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_CHANNEL_NAME_SIZE);
            break;
        case 8: /* list boxes response */
            /*
             * 1. code (uint8_t)
             * 2. last (uint8_t)
             * 3. box_name (char[32])
             * 4. box_size (uint64_t)
             * 5. n_publishers (uint64_t)
             * 6. n_subscribers (uint64_t)
             */
            memwrite_to_channel(fd, &code);
            memwrite_to_channel(fd, va_arg(ap, uint8_t*));
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_BOX_NAME_SIZE);
            memwrite_to_channel(fd, va_arg(ap, uint64_t*));
            memwrite_to_channel(fd, va_arg(ap, uint64_t*));
            memwrite_to_channel(fd, va_arg(ap, uint64_t*));
            break;
        case 9:  /* send message (publisher to sender) */
        case 10: /* send message (server to subscriber) */
            /*
             * 1. code (uint8_t)
             * 2. message (char[1024])
             */
            memwrite_to_channel(fd, &code);
            strwrite_to_channel(fd, va_arg(ap, char*), 1024);
            break;
        default: /* invalid operation code */
            break;
    }
}

void send_quick_message(const char *name, uint8_t code, ...) {
    int fd = open_channel(name, O_WRONLY);

    va_list ap;
    va_start(ap, code);
    vsend_message(fd, code, ap);
    va_end(ap);

    close_channel(fd);
}

void write_message(int fd, uint8_t code, ...) {
    va_list ap;
    va_start(ap, code);
    vsend_message(fd, code, ap);
    va_end(ap);
}

uint8_t read_code(int fd) {
    uint8_t code; /* returns -1 in case of EOF */
    return fread_from_channel(fd, &code, sizeof(uint8_t)) ? code : (0);
}

void read_content(int fd, uint8_t code, ...) {
    va_list ap;
    va_start(ap, code);

    switch (code) {
        case 1:
        case 2:
        case 3:
        case 5:
            fread_from_channel(fd, va_arg(ap, char*), 256);
            fread_from_channel(fd, va_arg(ap, char*), 32);
            break;
        case 4:
        case 6:
            fread_from_channel(fd, va_arg(ap, int32_t*), sizeof(int32_t));
            fread_from_channel(fd, va_arg(ap, char*), 1024);
            break;
        case 7:
            fread_from_channel(fd, va_arg(ap, char*), 256);
            break;
        case 8:
            fread_from_channel(fd, va_arg(ap, uint8_t*), sizeof(uint8_t));
            fread_from_channel(fd, va_arg(ap, char*), 32);
            fread_from_channel(fd, va_arg(ap, uint64_t*), sizeof(uint64_t));
            fread_from_channel(fd, va_arg(ap, uint64_t*), sizeof(uint64_t));
            fread_from_channel(fd, va_arg(ap, uint64_t*), sizeof(uint64_t));
            break;
        case 9:
        case 10:
            fread_from_channel(fd, va_arg(ap, char*), 1024);
            break;
        default:
            break;
    }

    va_end(ap);
}
