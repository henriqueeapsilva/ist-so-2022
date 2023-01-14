#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "box.h"
#include <stdlib.h>
#include <sys/types.h>

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
 * Writes the first `len` buffer bytes to a channel (uses write(3) syscall).
 *
 * Input:
 *   - fd: file descriptor
 *   - buffer: the content to write
 *   - len: how many bytes to write
 *
 * Returns the number of bytes written, or -1 in case of error.
 */
ssize_t channel_write(int fd, void *buffer, size_t len);

/**
 * Reads the next `len` bytes available in the channel (uses read(3)
 * syscall).
 *
 * Input:
 *   - fd: file descriptor
 *   - buffer: where the content will be stored
 *   - len: how many bytes to read
 *
 * Returns the number of bytes read, or -1 in case of error.
 */
ssize_t channel_read(int fd, void *buffer, size_t len);

#endif // _CHANNEL_H_