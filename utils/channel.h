#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

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