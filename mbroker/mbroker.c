#include <stdio.h>
#include "../utils/channel.h"
#include "../utils/logging.h"
#include "../utils/thread.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

#define SUCCESS_REG "OK!\n"

int main(int argc, char **argv) {
    if(argc < 3 || argc > 3) {
        fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions> \n");
        return EXIT_SUCCESS;
    }
    //int* num_sessions = (int *)argv[2];
    //pthread_t tid[*num_sessions];

    // Create register pipe

    create_channel(argv[1], 0640);

    // register channel receives a request
    int fregister = open_channel(argv[1], O_RDONLY);
    char buffer[1024];
    uint8_t code;

    while (1)
    {
        read_from_channel(fregister, &code, sizeof(code));
        switch (code) {
            case 1:
            case 2:
                read_from_channel(fregister, buffer, 256);
                int pi = open_channel(buffer, O_WRONLY);
                strwrite_to_channel(pi, SUCCESS_REG, sizeof(SUCCESS_REG));
                close_channel(pi);
                read_from_channel(fregister, buffer, 32);
                // create box with name on buffer
                break;
            case 3:
            case 5: //similar
                read_from_channel(fregister, buffer, 256);
                int p = open_channel(buffer, O_WRONLY);
                // return the 4 format answer (0 -> sucess, -1 -> error, error_message é enviado caso haja erro)
                close_channel(p);
                read_from_channel(fregister, buffer, 32);
                break;
            case 7:
                read_from_channel(fregister, buffer, 256);
                // returns 8 format message
                break;
            case 9:
                read_from_channel(fregister, buffer, 1024);
                // writes the message on the box_name of publisher
                break;
            default:
                break;
        }

    }
}
