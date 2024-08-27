#include "cli.h"
#include "./../../lib/DPS/dps_master.h"
#include "../can_lib/canlib.h"
#include <stdio.h>
#include <stdlib.h>


//private
int parser(char* mex){
    return EXIT_SUCCESS;
}

int send_mex(CanMessage *mex){
    return 0;
}

//public
int cli_init()
{
    if (dps_master_init(send_mex)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
int cli_start()
{
    char can_interface[1024] = {};
    printf("insert the name of the interface [max 1024]:");
    fflush(stdin);
    fscanf(stdin, "%s", can_interface);
    fflush(stdin);
    if (can_init(can_interface)) {
        fprintf(stderr, "failed init can interface: %s\n",can_interface);
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}
