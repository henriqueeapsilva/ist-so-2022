#include "fifo.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int pipe; // file descriptor for manager pipe
int reg;  // file descriptor for register pipe

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe> <pipe_name> list\n");
}

static void create_box(int argc, char **argv) {

}

static void remove_box(int argc, char **argv) {

}

static void list() {

}

int verify_entry(char* mode){
    if(!strcmp(mode, "create")) return 0;
    if(!strcmp(mode, "remove")) return 1;
    if(!strcmp(mode, "list")) return 2;
    return -1;
}

int main(int argc, char **argv) {
    if(argc < 4) {
        print_usage();
        return(EXIT_FAILURE);
    }

    // Create pipe
    fifo_make(argv[2], 0640);

    pipe = fifo_open(argv[2], O_WRONLY);
    reg = fifo_open(argv[1], O_WRONLY);

    if (!strcmp("create", argv[3])) {
        create_box(argc, argv);
    } else if (!strcmp("remove", argv[3])) {
        remove_box(argc, argv);
    } else if (!strcmp("list", argv[3])) {
        list();
    } else {
        print_usage();
    }

    fifo_close(reg);
    fifo_close(pipe);
    fifo_unlink(argv[2]);

    reg = NULL;
    pipe = NULL;
    
    return EXIT_SUCCESS;
}
