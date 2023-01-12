#ifndef _BOXES_H_
#define _BOXES_H_

#include "../utils/box.h"

int init_boxes();

int destroy_boxes();

void create_box(char *channel_name, char *box_name);

void delete_box(char *channel_name, char *box_names);

void list_boxes(char *channel_name);

void register_pub(char *channel_name);

void register_sub(char *channel_name);

#endif