#include "channel.h"
#include "protocol.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void serialize_message(void *buffer, uint8_t code, ...) {
    va_list ap;
    va_start(ap, code);

    size_t len;
    memcpy(buffer, &code, (len = sizeof(uint8_t)));
    buffer += len;

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
        strncpy(buffer, va_arg(ap, char *), (len = sizeof(char) * 256));
        buffer += len;
        strncpy(buffer, va_arg(ap, char *), (len = sizeof(char) * 32));
        break;
    case OP_CREATE_BOX_RET:
    case OP_REMOVE_BOX_RET:
        /*
         * 1. code (uint8_t)
         * 2. error_code (int32_t)
         * 3. error_message (char[1024])
         */
        memcpy(buffer, va_arg(ap, int32_t *), (len = sizeof(int32_t)));
        buffer += len;
        strncpy(buffer, va_arg(ap, char *), (len = sizeof(char) * 1024));
        break;
    case OP_LIST_BOXES:
        /*
         * 1. code (uint8_t)
         * 2. client_named_pipe_path (char[256])
         */
        strncpy(buffer, va_arg(ap, char *), (len = sizeof(char) * 256));
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
        memcpy(buffer, va_arg(ap, uint8_t *), (len = sizeof(uint8_t)));
        buffer += len;
        strncpy(buffer, va_arg(ap, char *), (len = sizeof(char) * 32));
        buffer += len;
        memcpy(buffer, va_arg(ap, uint64_t *), (len = sizeof(uint64_t)));
        buffer += len;
        memcpy(buffer, va_arg(ap, uint64_t *), (len = sizeof(uint64_t)));
        buffer += len;
        memcpy(buffer, va_arg(ap, uint64_t *), (len = sizeof(uint64_t)));
        break;
    case OP_MSG_PUB_TO_SER:
    case OP_MSG_SER_TO_SUB:
        /*
         * 1. code (uint8_t)
         * 2. message (char[1024])
         */
        strncpy(buffer, va_arg(ap, char *), (len = sizeof(char) * 1024));
        break;
    default:
        // invalid operation code
        break;
    }

    va_end(ap);
}

void deserialize_message(void *buffer, uint8_t code, ...) {
    va_list ap;
    va_start(ap, code);

    buffer += sizeof(uint8_t);

    switch (code) {
    case OP_REGISTER_PUB:
    case OP_REGISTER_SUB:
    case OP_CREATE_BOX:
    case OP_REMOVE_BOX:
        strcpy(va_arg(ap, char *), buffer);
        buffer += 256;
        strcpy(va_arg(ap, char *), buffer);
        break;
    case OP_CREATE_BOX_RET:
    case OP_REMOVE_BOX_RET:
        memcpy(va_arg(ap, int32_t *), buffer, sizeof(int32_t));
        buffer += sizeof(int32_t);
        strcpy(va_arg(ap, char *), buffer);
        break;
    case OP_LIST_BOXES:
        strcpy(va_arg(ap, char *), buffer);
        break;
    case OP_LIST_BOXES_RET:
        memcpy(va_arg(ap, uint8_t*), buffer, sizeof(uint8_t));
        buffer += sizeof(uint8_t);
        strcpy(va_arg(ap, char *), buffer);
        buffer += 32;
        memcpy(va_arg(ap, uint64_t*), buffer, sizeof(uint64_t));
        buffer += sizeof(uint64_t);
        memcpy(va_arg(ap, uint64_t*), buffer, sizeof(uint64_t));
        buffer += sizeof(uint64_t);
        memcpy(va_arg(ap, uint64_t*), buffer, sizeof(uint64_t));
        break;
    case OP_MSG_PUB_TO_SER:
    case OP_MSG_SER_TO_SUB:
        strcpy(va_arg(ap, char *), buffer);
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
