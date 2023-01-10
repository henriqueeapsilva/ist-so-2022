#include <stdlib.h>
#include "../mbroker/mbroker.c"

int find_free_box(){
    for (int i = 0; i < NUM_BOXES; i++) {
        if(boxes[i].name == NULL)
            return i;
    }
    return -1;
}

int find_named_box(const char *name){
    for (int i = 0; i < NUM_BOXES; i++) {
        if(strcmp(boxes[i].name, name))
            return i;
    }
    return -1;
}



