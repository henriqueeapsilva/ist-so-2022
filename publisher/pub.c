#include "../utils/logging.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/fifo.h"
int main(int argc, char **argv) {

    // create session fifo
    fifo_make(argv[2], 0640);

    int fregister = fifo_open(argv[1], O_WRONLY);

    char msg[sizeof(u_int8_t) + 256 + 32];

    sprintf(msg, "%d", 1);      // insert request code
    strncpy(msg+1, argv[2], 256);    // insert session pipe name
    strncpy(msg+1+256, argv[4], 32); // insert box name

    fifo_send_msg(fregister, msg);
    fifo_close(fregister);

    int fsession = fifo_open(argv[2], O_RDONLY);
    char buffer[1024];
    int readed = fifo_read(fsession,buffer, 1024);
    // we have to decide on the mbroker which message is sent as it's ok to read or not
    /*
     * code to confirm if it's ready
     * */
    fifo_close(fsession);

    int fs = fifo_open(argv[2], O_WRONLY);
    char buf[1024];
    int c;
    while((c=getchar()) != EOF){
    }

    fifo_close(fs);

    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
