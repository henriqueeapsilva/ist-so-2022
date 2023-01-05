#include "../utils/logging.h"
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


int verify_entry(char* mode){
    if(strcmp(mode, "create") == 0) return 0;
    if(strcmp(mode, "remove") == 0) return 1;
    if(strcmp(mode, "list") == 0) return 2;
    return -1;
}

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    if(argc < 4) {
        print_usage();
        return(EXIT_FAILURE);
    }
    // Create pipe
    if (mkfifo(argv[2], 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int pipe = open(argv[2], O_WRONLY);  // manager pipe
    if (pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int reg = open(argv[1], O_WRONLY);  // register pipe
    if (reg == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    switch (verify_entry(argv[3])) {
        case 0:
            
            break;
        case 1:
            break;
        case 2:
            break;
        case -1:
            print_usage();
            exit(EXIT_FAILURE);
    }

    if(close(reg) == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(close(pipe) == -1){
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2], strerror(errno));
        exit(EXIT_FAILURE);
    }
    WARN("unimplemented"); // TODO: implement
    return EXIT_SUCCESS;
}
