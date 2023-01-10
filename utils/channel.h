#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_CHANNEL_NAME_SIZE (256)
#define MAX_MESSAGE_SIZE (1024)


typedef struct {
    uint8_t code;

    union {
        struct {
            char channel_name[256];

            union {
                char box_name[32];
                struct {};
            };
        };

        struct {
            char message[1024];

            union {
                int32_t return_code;
                struct {};
            };
        };

        struct {
            uint8_t last;
            char box_name[32];
            uint64_t box_size;
            uint64_t n_pubs;
            uint64_t n_subs;
        };

        struct {};
    };

} Message;

/**
 * A channel allows two different threads/processes to easily
 * communicate with each other without too much overhead.
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
 * - Create box response:
 *  [ code = 4 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
 * - Remove box request:
 *  [ code = 5 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * - Remove box response:
 *  [ code = 6 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
 * - List boxes request:
 *  [ code = 7 (uint8_t) ] | [ client_named_pipe_path (char[256]) ]
 * - List boxes response:
 *  [ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
 * - Message (publisher to server)
 *  [ code = 9 (uint8_t) ] | [ message (char[1024]) ]
 * - Message (server to subscriber)
 *  [ code = 10 (uint8_t) ] | [ message (char[1024]) ]
 */

/**
 * Creates a channel (uses mkfifo syscall).
 * 
 * Input:
 *   - name: name of the channel
 *   - mode: access permissions (4 numbers representing 2 bytes, eg: 0640)
*/
void create_channel(const char *name, mode_t mode);

/**
 * Deletes a channel (uses unlink syscall).
 * 
 * Input:
 *   - name: name of the channel
 */
void delete_channel(const char *name);

/**
 * Opens a channel (uses open syscall).
 * 
 * Input:
 *   - name: name of the channel
 *   - flags: determines its use (reading/writing)
 */
int open_channel(const char *name, int flags);

/**
 * Closes a channel (uses close syscall).
 * 
 * Input:
 *   - fd: file descriptor
 */
void close_channel(int fd);

/**
 * Sends a message to a channel (uses write syscall).
 * 
 * Input:
 *   - fd: file descriptor
 *   - code: operation code (determines how many arguments must be available)
 *   - ...: message elements to be serialized and sent
 */
void write_message(int fd, uint8_t code, ...);

/**
 * Receives the code of the next message available in the channel (uses read syscall).
 * 
 * Input:
 *   - fd: file descriptor
 * 
 * Returns the code of the next message
 */
uint8_t read_code(int fd);

/**
 * Receives the content of the message being read
 */
void read_content(int fd, uint8_t code, ...);

#endif