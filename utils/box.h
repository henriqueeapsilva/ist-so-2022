#ifndef _BOX_H_
#define _BOX_H_

#include <stdint.h>

#define MAX_BOX_COUNT (64)
#define MAX_BOX_NAME_SIZE (32)

typedef struct {
    char name[MAX_BOX_NAME_SIZE];
    uint64_t size;
    uint64_t n_pubs;
    uint64_t n_subs;
} Box;

#endif