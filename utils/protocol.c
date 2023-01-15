#include "channel.h"
#include "protocol.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static size_t _memcpy(void *dest, void *src) {
    size_t n = sizeof(src);
    memcpy(dest, src, n);
    return n;
}

static size_t _strncpy(void *dest, void *src, size_t n) {
    strncpy(dest, src, n);
    return sizeof(char) * n;
}

void serialize_message(void *buffer, uint8_t code, ...) {
    buffer += _memcpy(buffer, &code);

    va_list ap;
    va_start(ap, code);

    switch (code) {
    case OP_REGISTER_PUB:
    case OP_REGISTER_SUB:
    case OP_CREATE_BOX:
    case OP_REMOVE_BOX: {
        char *channel = va_arg(ap, char*);
        char *box = va_arg(ap, char*);

        buffer += _strncpy(buffer, channel, 256);
        buffer += _strncpy(buffer, box, 32);
    } break;
    case OP_CREATE_BOX_RET:
    case OP_REMOVE_BOX_RET: {
        int32_t error_code = va_arg(ap, int32_t);
        char *error_message = va_arg(ap, char*);

        buffer += _memcpy(buffer, &error_code);
        buffer += _strncpy(buffer, error_message, 1024);
    } break;
    case OP_LIST_BOXES: {
        char *box = va_arg(ap, char*);
        
        strncpy(buffer, box, 256);
    } break;
    case OP_LIST_BOXES_RET: {
        uint8_t last = va_arg(ap, uint8_t);
        char *box = va_arg(ap, char*);
        uint64_t size = va_arg(ap, uint64_t);
        uint64_t n_pubs = va_arg(ap, uint64_t);
        uint64_t n_subs = va_arg(ap, uint64_t);

        buffer += _memcpy(buffer, &last);
        buffer += _strncpy(buffer, box, 32);
        buffer += _memcpy(buffer, &size);
        buffer += _memcpy(buffer, &n_pubs);
        buffer += _memcpy(buffer, &n_subs);
    } break;
    case OP_MSG_PUB_TO_SER:
    case OP_MSG_SER_TO_SUB: {
        char *message = va_arg(ap, char*);

        strncpy(buffer, message, 1024);
    } break;
    default: // invalid operation code
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
    case OP_REMOVE_BOX: {
        char *channel = va_arg(ap, char*);
        char *box = va_arg(ap, char*);

        buffer += _strncpy(channel, buffer, 256);
        buffer += _strncpy(box, buffer, 32);
    } break;
    case OP_CREATE_BOX_RET:
    case OP_REMOVE_BOX_RET: {
        int32_t error_code = va_arg(ap, int32_t);
        char *error_message = va_arg(ap, char*);

        buffer += _memcpy(&error_code, buffer);
        buffer += _strncpy(error_message, buffer, 1024);
    } break;
    case OP_LIST_BOXES: {
        char *box = va_arg(ap, char*);
        
        buffer += _strncpy(box, buffer, 256);
    } break;
    case OP_LIST_BOXES_RET: {
        uint8_t last = va_arg(ap, uint8_t);
        char *box = va_arg(ap, char*);
        uint64_t size = va_arg(ap, uint64_t);
        uint64_t n_pubs = va_arg(ap, uint64_t);
        uint64_t n_subs = va_arg(ap, uint64_t);

        buffer += _memcpy(&last, buffer);
        buffer += _strncpy(box, buffer, 32);
        buffer += _memcpy(&size, buffer);
        buffer += _memcpy(&n_pubs, buffer);
        buffer += _memcpy(&n_subs, buffer);
    } break;
    case OP_MSG_PUB_TO_SER:
    case OP_MSG_SER_TO_SUB: {
        char *message = va_arg(ap, char*);

        buffer += _strncpy(message, buffer, 1024);
    } break;
    default: // invalid operation code
        break;
    }

    va_end(ap);
}

uint8_t deserialize_code(void *buffer) {
    uint8_t code;
    memcpy(&code, buffer, sizeof(uint8_t));
    return code;
}
