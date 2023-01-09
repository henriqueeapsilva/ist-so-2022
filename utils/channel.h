#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

void create_channel(const char *name, mode_t mode);
void delete_channel(const char *name);

int open_channel(char *name, int flags);
void close_channel(int fd);

size_t write_to_channel(int fd, const void *buffer, size_t len);
size_t read_from_channel(int fd, void *buffer, size_t len);

/**
 * Force write to the channel.
 * Retries to send whatever was not sent in the beginning.
 */
void fwrite_to_channel(int fd, const void *buffer, size_t len);

/**
 * Force read from the channel.
 * Retries to receive whatever was not received in the beginning.
 */
void fread_from_channel(int fd, void *buffer, size_t len);

/**
 * Writes a piece of data to the channel.
 */
void memwrite_to_channel(int fd, void *ptr);

/**
 * Writes a string to the channel, filling the end with '\0'.
 */
void strwrite_to_channel(int fd, char *string, size_t len);

void send_protocol_message(int fd, uint8_t code, ...);

#endif