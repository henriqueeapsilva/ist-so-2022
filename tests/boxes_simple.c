#include "thread.h"
#include <stdlib.h>
#include <unistd.h>

void exec_mbroker(char *reg_channel_name) {
    execl("../mbroker/mbroker.c", "mbroker", reg_channel_name, 20, 0);
}

void exec_publisher(char *reg_channel_name, char *box_name) {
    execl("../publisher/pub.c", "mbroker", reg_channel_name, box_name, 0);
}

void exec_subscriber(char *reg_channel_name, char *box_name) {
    execl("../subscriber/sub.c", "mbroker", reg_channel_name, box_name, 0);
}

void list_boxes(char *reg_channel_name, char *channel_name) {
    execl("../manager/manager.c", "manager", reg_channel_name, channel_name,
          "list", 0);
}

void create_box(char *reg_channel_name, char *channel_name, char *box_name) {
    execl("../manager/manager.c", "manager", reg_channel_name, channel_name,
          "create", box_name, 0);
}

void remove_box(char *reg_channel_name, char *channel_name, char *box_name) {
    execl("../manager/manager.c", "manager", reg_channel_name, channel_name,
          "remove", box_name, 0);
}

int main(void) { return EXIT_SUCCESS; }