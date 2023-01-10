#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

#define MAX_CHANNEL_NAME 256
#define MAX_STRING_SIZE 1024

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

void create_channel(const char *name, mode_t mode);
void delete_channel(const char *name);

int open_channel(const char *name, int flags);
void close_channel(int fd);

/**
 * Sends a protocol message to the given channel.
 * There are 10 valid operations:
 */
void send_message(int fd, uint8_t code, ...);

/**
 * Sends a quick protocol message to the given channel (which is currently closed).
 * The channel will be open so that the message can be sent, then it will be closed.
 */
void send_quick_message(const char *name, uint8_t code, ...);

uint8_t receive_code(int fd);

void receive_content(int fd, uint8_t code, ...);

#endif