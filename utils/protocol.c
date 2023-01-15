#include "channel.h"
#include "protocol.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static size_t _memcpy(void *dest, void *src) {
    size_t n = sizeof(*src);
    memcpy(dest, src, n);
    return n;
}

static size_t _strncpy(void *dest, void *src, size_t n) {
    strncpy(dest, src, n);
    return sizeof(char) * n;
}

static size_t _strcpy(void *dest, void *src, size_t n) {
    strcpy(dest, src);
    return sizeof(char) * n;
}

void serialize_message(void *buffer, uint8_t code, ...) {
    buffer += _memcpy(buffer, &code);

    va_list ap;
    va_start(ap, code);

    buffer += _memcpy(buffer, &code);

    switch (code) {
    case OP_REGISTER_PUB:
    case OP_REGISTER_SUB:
    case OP_CREATE_BOX:
    case OP_REMOVE_BOX:
        /*
         * 1. code (uint8_t)
         * 2. client_named_pipe_path (char[256])
         * 3. box_name (char[32])
         */
        buffer += _strncpy(buffer, va_arg(ap, char *), 256);
        buffer += _strncpy(buffer, va_arg(ap, char *), 32);
        break;
    case OP_CREATE_BOX_RET:
    case OP_REMOVE_BOX_RET:
        /*
         * 1. code (uint8_t)
         * 2. error_code (int32_t)
         * 3. error_message (char[1024])
         */
        buffer += _memcpy(buffer, va_arg(ap, int32_t *));
        buffer += _strncpy(buffer, va_arg(ap, char *), 1024);
        break;
    case OP_LIST_BOXES:
        /*
         * 1. code (uint8_t)
         * 2. client_named_pipe_path (char[256])
         */
        buffer += _strncpy(buffer, va_arg(ap, char *), 256);
        break;
    case OP_LIST_BOXES_RET:
        /*
         * 1. code (uint8_t)
         * 2. last (uint8_t)
         * 3. box_name (char[32])
         * 4. box_size (uint64_t)
         * 5. n_publishers (uint64_t)
         * 6. n_subscribers (uint64_t)
         */
        buffer += _memcpy(buffer, va_arg(ap, uint8_t *));
        buffer += _strncpy(buffer, va_arg(ap, char *), 32);
        buffer += _memcpy(buffer, va_arg(ap, uint64_t *));
        buffer += _memcpy(buffer, va_arg(ap, uint64_t *));
        buffer += _memcpy(buffer, va_arg(ap, uint64_t *));
        break;
    case OP_MSG_PUB_TO_SER:
    case OP_MSG_SER_TO_SUB:
        /*
         * 1. code (uint8_t)
         * 2. message (char[1024])
         */
        buffer += _strncpy(buffer, va_arg(ap, char *), 1024);
        break;
    default:
        // invalid operation code
        break;
    }

    va_end(ap);
}

void deserialize_message(void *buffer, uint8_t code, ...) {
    buffer += sizeof(uint8_t);

    va_list ap;
    va_start(ap, code);

    switch (code) {
    case OP_REGISTER_PUB:
    case OP_REGISTER_SUB:
    case OP_CREATE_BOX:
    case OP_REMOVE_BOX:
        buffer += _strcpy(va_arg(ap, char *), buffer, 256);
        buffer += _strcpy(va_arg(ap, char *), buffer, 32);
        break;
    case OP_CREATE_BOX_RET:
    case OP_REMOVE_BOX_RET:
        buffer += _memcpy(va_arg(ap, int32_t *), buffer);
        buffer += _strcpy(va_arg(ap, char *), buffer, 1024);
        break;
    case OP_LIST_BOXES:
        buffer += _strcpy(va_arg(ap, char *), buffer, 256);
        break;
    case OP_LIST_BOXES_RET:
        buffer += _memcpy(va_arg(ap, uint8_t*), buffer);
        buffer += _strcpy(va_arg(ap, char *), buffer, 32);
        buffer += _memcpy(va_arg(ap, uint64_t*), buffer);
        buffer += _memcpy(va_arg(ap, uint64_t*), buffer);
        buffer += _memcpy(va_arg(ap, uint64_t*), buffer);
        break;
    case OP_MSG_PUB_TO_SER:
    case OP_MSG_SER_TO_SUB:
        buffer += _strcpy(va_arg(ap, char *), buffer, 1024);
        break;
    default:
        break;
    }

    va_end(ap);
}

uint8_t deserialize_code(void *buffer) {
    uint8_t code;
    memcpy(&code, buffer, sizeof(uint8_t));
    return code;
}
