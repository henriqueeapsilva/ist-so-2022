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

void channel_create(const char *name, mode_t mode) {
    if (mkfifo(name, mode) == -1) {
        fprintf(stderr, "Could not create fifo: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void channel_delete(const char *name) {
    if ((unlink(name) == -1) && (errno != ENOENT)) {
        fprintf(stderr, "Could not unlink fifo (%s): %s\n", name, strerror(errno));
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

static void memwrite_to_channel(int fd, void *ptr) {
    fwrite_to_channel(fd, ptr, sizeof(ptr));
}

static void strwrite_to_channel(int fd, char *string, size_t len) {
    char buffer[len];
    buffer[len - 1] = '\0';
    fwrite_to_channel(fd, strncpy(buffer, string, len - 1), len);
}

void channel_write(int fd, uint8_t code, ...) {
    va_list ap;
    va_start(ap, code);

    switch (code) {
        case REGISTER_PUB:
        case REGISTER_SUB:
        case CREATE_BOX:
        case REMOVE_BOX:
            /*
             * 1. code (uint8_t)
             * 2. client_named_pipe_path (char[256])
             * 3. box_name (char[32])
             */
            memwrite_to_channel(fd, &code);
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_CHANNEL_NAME_SIZE);
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_BOX_NAME_SIZE);
            break;
        case CREATE_BOX_RET:
        case REMOVE_BOX_RET:
            /*
             * 1. code (uint8_t)
             * 2. error_code (int32_t)
             * 3. error_message (char[1024])
             */
            memwrite_to_channel(fd, &code);
            memwrite_to_channel(fd, va_arg(ap, int32_t*));
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_MESSAGE_SIZE);
            break;
        case LIST_BOXES:
            /*
             * 1. code (uint8_t)
             * 2. client_named_pipe_path (char[256])
             */
            memwrite_to_channel(fd, &code);
            strwrite_to_channel(fd, va_arg(ap, char*), MAX_CHANNEL_NAME_SIZE);
            break;
        case LIST_BOXES_RET:
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
        case MSG_PUB_TO_SER:
        case MSG_SER_TO_SUB:
            /*
             * 1. code (uint8_t)
             * 2. message (char[1024])
             */
            memwrite_to_channel(fd, &code);
            strwrite_to_channel(fd, va_arg(ap, char*), 1024);
            break;
        default:
            // invalid operation code
            break;
    }

    va_end(ap);
}

uint8_t channel_read_code(int fd) {
    uint8_t code; // returns -1 in case of EOF
    return fread_from_channel(fd, &code, sizeof(uint8_t)) ? code : (0);
}

void channel_read_content(int fd, uint8_t code, ...) {
    va_list ap;
    va_start(ap, code);
    
    switch (code) {
        case REGISTER_PUB:
        case REGISTER_SUB:
        case CREATE_BOX:
        case REMOVE_BOX:
            fread_from_channel(fd, va_arg(ap, char*), 256);
            fread_from_channel(fd, va_arg(ap, char*), 32);
            break;
        case CREATE_BOX_RET:
        case REMOVE_BOX_RET:
            fread_from_channel(fd, va_arg(ap, int32_t*), sizeof(int32_t));
            fread_from_channel(fd, va_arg(ap, char*), 1024);
            break;
        case LIST_BOXES:
            fread_from_channel(fd, va_arg(ap, char*), 256);
            break;
        case LIST_BOXES_RET:
            fread_from_channel(fd, va_arg(ap, uint8_t*), sizeof(uint8_t));
            fread_from_channel(fd, va_arg(ap, char*), 32);
            fread_from_channel(fd, va_arg(ap, uint64_t*), sizeof(uint64_t));
            fread_from_channel(fd, va_arg(ap, uint64_t*), sizeof(uint64_t));
            fread_from_channel(fd, va_arg(ap, uint64_t*), sizeof(uint64_t));
            break;
        case MSG_PUB_TO_SER:
        case MSG_SER_TO_SUB:
            fread_from_channel(fd, va_arg(ap, char*), 1024);
            break;
        default:
            break;
    }

    va_end(ap);
}
