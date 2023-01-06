#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdlib.h>

void fifo_send_msg(int fd, const char *buffer);

void fifo_make(const char *name, mode_t mode);
void fifo_unlink(const char *name);

int fifo_open(const char *name, int flags);
void fifo_close(int fd);

ssize_t fifo_read(int fd, const void *buffer, size_t count);
ssize_t fifo_write(int fd, const void *buffer, size_t count);

#endif