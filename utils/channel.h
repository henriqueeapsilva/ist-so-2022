#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_CHANNEL_NAME_SIZE (256)
#define MAX_MESSAGE_SIZE (1024)

/*
 * A channel allows two different threads/processes to easily
 * communicate with each other without too much of overhead.
 * 
 * To moderate the interaction between server and clients, it was
 * established a protocol to standardize the way messages are built
 * inside a buffer of bytes.
 * 
 * The content of each message should follow a format where:
 * - The '|' symbol denotes a concatenation of message elements.
 * - All messages begin with a code that identifies the intended operation (OP_CODE).
 * - Strings have a fixed length and their endings must be filled with '\0'.
 * 
 * This protocol supports the following operations:
 * - Publisher registration request:
 *  [ code = 1 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * - Subscriber registration request:
 *  [ code = 2 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * - Create box request:
 *  [ code = 3 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * - Create box return:
 *  [ code = 4 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
 * - Remove box request:
 *  [ code = 5 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * - Remove box return:
 *  [ code = 6 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
 * - List boxes request:
 *  [ code = 7 (uint8_t) ] | [ client_named_pipe_path (char[256]) ]
 * - List boxes return:
 *  [ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
 * - Message (publisher to server)
 *  [ code = 9 (uint8_t) ] | [ message (char[1024]) ]
 * - Message (server to subscriber)
 *  [ code = 10 (uint8_t) ] | [ message (char[1024]) ]
 */

typedef enum __attribute__((__packed__)) op_code {
    REGISTER_PUB=1, // register publisher request
    REGISTER_SUB,   // register subscriber request
    CREATE_BOX,     // create box request
    CREATE_BOX_RET, // create box response/return
    REMOVE_BOX,     // remove box request
    REMOVE_BOX_RET, // remove box response/return
    LIST_BOXES,     // list boxes request
    LIST_BOXES_RET, // list boxes response/return
    MSG_PUB_TO_SER, // send message (from publisher to server)
    MSG_SER_TO_SUB, // send message (from subscriber to server)
};

/**
 * Creates a channel (uses mkfifo(2) syscall).
 * 
 * Input:
 *   - name: name of the channel
 *   - mode: access permissions (4 numbers representing 2 bytes, eg: 0640)
 * 
 * In case of error, the program will quit.
*/
void channel_create(const char *name, mode_t mode);

/**
 * Deletes a channel (uses unlink(1) syscall).
 * 
 * Input:
 *   - name: name of the channel
 * 
 * In case of error, the program will quit.
 */
void channel_delete(const char *name);

/**
 * Opens a channel (uses open(2) syscall).
 * 
 * Input:
 *   - name: name of the channel
 *   - flags: determines its use (reading/writing)
 * 
 * Returns the file descriptor.
 * In case of error, the program will quit.
 */
int channel_open(const char *name, int flags);

/**
 * Closes a channel (uses close(1) syscall).
 * 
 * Input:
 *   - fd: file descriptor
 * 
 * In case of error, the program will quit.
 */
void channel_close(int fd);

/**
 * Writes a message to a channel (uses write(3) syscall).
 * The message is serialized accordingly to the protocol.
 * 
 * Input:
 *   - fd: file descriptor
 *   - code: operation code (determines how many arguments must be given)
 *   - ...: message elements to be serialized and sent
 * 
 * In case of error, the program will quit.
 */
void channel_write(int fd, uint8_t code, ...);

/**
 * Reads a message from the channel (uses read(3) syscall).
 * The message is deserialized accordingly to the protocol.
 * If the code read is not the same as the one given, the program will quit.
 * 
 * Input:
 *   - fd: file descriptor
 *   - code: operation code (determines how many arguments must be given)
 * 
 * In case of error, the program will quit.
 */
void channel_read(int fd, uint8_t code, ...);

/**
 * Reads the content of the next message available in the channel (uses read(3) syscall).
 * The message is deserialized accordingly to the protocol.
 * channel_read_code(1) must be called first.
 * 
 * Input:
 *   - fd: file descriptor
 *   - code: the code returned by channel_read_code(1)
 * 
 * In case of error, the program will quit.
 */
void channel_read_content(int fd, uint8_t code, ...);

/**
 * Reads the code of the next message available in the channel (uses read(3) syscall).
 * The code lets the client know what kind of message he is receiving.
 * 
 * Input:
 *   - fd: file descriptor
 * 
 * Returns the code of the next message, or 0 if EOF was reached (no more writers/publishers available).
 * In case of error, the program will quit.
 */
uint8_t channel_read_code(int fd);

#endif