#include "cli.h"
#include "./../../lib/DPS/dps_master.h"
#include "../can_lib/canlib.h"
#include <stdio.h>
#include <stdlib.h>


//private

int socket_can=0;


int parser(){
    char buffer[1024] = {};
    buffer[0] = getchar();
    switch (buffer[0]) {
        case 'n':
            printf("new connection\n");
            break;
        case 'u':
            printf("update info of board (one or all)\n");
            break;
        case 'b':
            printf("list boards\n");
            break;
        case 'v':
            printf("list vars\n");
            break;
        case 'c':
            printf("list coms\n");
            break;
        case 's':
            printf("send\n");
            break;
        case 'q':
            printf("quit\n");
            return 1;
        default:
            printf("b: list boards\n");
            printf("v: list vars\n");
            printf("c: list coms\n");
            printf("n: scan boards\n");
            printf("s: send mex (update var, command)\n");
            break;
    }
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
    {
        char can_interface[1024] = {};
        printf("insert the name of the interface [max 1024]:");
        fflush(stdin);
        fscanf(stdin, "%s", can_interface);
        fflush(stdin);
        socket_can = can_init(can_interface);
        if (!socket_can) {
            fprintf(stderr, "failed init can interface: %s\n",can_interface);
            return EXIT_FAILURE;
        }
    }

    while (1) {
        int out = parser();
        if (out > 0) {
            break;
        }else if(out < 0){
            goto crush;
        }
    }


    return EXIT_SUCCESS;

crush:
    return EXIT_FAILURE;
}
