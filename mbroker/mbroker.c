#include <stdio.h>
#include "../utils/channel.h"
#include "../utils/logging.h"
#include "../utils/thread.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <operations.h>
#include <string.h>
#include "../utils/box.h"

#define SUCCESS_REG "OK"
#define NUM_BOXES 64

typedef struct box {
    char *name;
    uint64_t size;
    uint64_t num_pub;
    uint64_t num_sub;
}Box;


static Box boxes[NUM_BOXES];

void register_pub(int fd, uint8_t code);
void register_sub(int fd, uint8_t code);
void create_box(int fd, uint8_t code);
void eliminate_box(int fd, uint8_t code);
void list_boxes(int fd, uint8_t code);


void *session(void *arg){
    char * pipe_name = (char *)arg;
    // register channel receives a request
    int fregister = open_channel(pipe_name, O_RDONLY);

    while(1){
        uint8_t code = receive_code(fregister);
        switch (code) {
            case 1:
                register_pub(fregister, code);
                break;
            case 2:
                register_sub(fregister, code);
                break;
            case 3:
                create_box(fregister, code);
                break;
            case 5:
                eliminate_box(fregister, code);
                break;
            case 7:
                list_boxes(fregister, code);
                break;
        }

    }
}


int main(int argc, char **argv) {
    if(argc < 3 || argc > 3) {
        fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions> \n");
        return EXIT_SUCCESS;
    }
    int* num_sessions = (int *)argv[2];
    pthread_t tid[*num_sessions];

    // tfs initialization
    tfs_init(NULL);


    // Create register pipe
    create_channel(argv[1], 0640);

    // create a thread (example)
    thread_create(&tid[0], session, &argv[1]);

    // join
    thread_join(&tid[0], NULL);

    tfs_destroy();
}

void register_pub(int fd, uint8_t code){
    char pipe_name[256];
    char box_name[32];
    receive_content(fd, code, pipe_name, box_name);
    int box_idx = find_named_box(box_name);
    if(box_idx == -1)
        send_quick_message(pipe_name, 9, "boxed not found!");

    int fsession = open_channel(pipe_name, O_RDONLY);
    int fhandle = tfs_open(box_name, TFS_O_CREAT | TFS_O_APPEND);
    boxes[box_idx].num_pub = 1;

    char message[1024];

    while((code = receive_code(fsession))){
        assert(code == 9);  // verifies the code != 9
        // receive the message
        receive_content(fsession, code, message);
        uint64_t num_char_msg = strlen(message);
        tfs_write(fhandle, message, num_char_msg + 1);
        boxes[box_idx].size += num_char_msg + 1;

    }

    close_channel(fsession);
    tfs_close(fhandle);
    boxes[box_idx].num_pub = 0;
}

void register_sub(int fd, uint8_t code){
    char pipe_name[256];
    char box_name[32];
    receive_content(fd, code, pipe_name, box_name);
    int box_idx = find_named_box(box_name);
    if(box_idx == -1)
        send_quick_message(pipe_name, 10, "box not found!");

    int fsession = open_channel(pipe_name, O_WRONLY);
    tfs_write(fsession, SUCCESS_REG, strlen(SUCCESS_REG));
    int fhandle = tfs_open(box_name, TFS_O_CREAT | TFS_O_APPEND);
    boxes[box_idx].num_sub++;

    char message[1024];

    while(1){
        tfs_read(fhandle, message, 1024);
        send_message(fsession, 10, message);
    }
}

void create_box(int fd, uint8_t code){
    char pipe_name[256];
    char box_name[32];

    receive_content(fd, code, pipe_name, box_name);
    if(find_named_box(box_name) != -1)
        send_quick_message(pipe_name, 4, 1, "box already exists!");

    int fhandle = tfs_open(box_name, TFS_O_CREAT);

    int fsession = open_channel(pipe_name, O_WRONLY);
    if(fhandle == -1)
        send_quick_message(pipe_name,4,1,"failed to open box!");

    send_quick_message(pipe_name, 4, 0, "box successfully created!");

    tfs_close(fhandle);
    close_channel(fsession);
}

void eliminate_box(int fd, uint8_t code){

}

void list_boxes(int fd, uint8_t code){

}