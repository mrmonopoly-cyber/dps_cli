#include "cli.h"
#include "./../../lib/DPS/dps_master.h"
#include "../can_lib/canlib.h"
#include <linux/can.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>


//private

static int socket_can=0;

static int get_board_input(){
    int8_t board_id = -1;
    printf("board id [digit]: ");
    fflush(stdin);
    fflush(stdout);
    scanf("%s\n",&board_id);
    fflush(stdin);
    fflush(stdout);

    return board_id;
}

static int parser(){
    char buffer = ' ';
    int8_t board_id = -1;
    buffer  = getchar();
    switch (buffer) {
        case 'n':
            printf("new connection\n");
            if (dps_master_new_connection()) {
                return -1;
            }
            break;
        case 'u':
            printf("update info of one board \n");
            board_id = get_board_input();
            if (board_id < 0) {
                break;
            }
            dps_master_refresh_value_var_all(board_id);
            break;
        case 'b':
            printf("list boards\n");
            break;
        case 'v':
            printf("list vars\n");
            board_id = get_board_input();
            if (board_id < 0) {
                break;
            }
            var_list_info* vars = NULL;
            if(dps_master_list_vars(board_id, &vars)){
                break;
            }
            for (uint8_t i =0; i<vars->board_num; i++) {           
            }
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
            printf("n: scan boards\n");
            printf("u: update info boards (one or all)\n");
            printf("b: list boards\n");
            printf("v: list vars\n");
            printf("c: list coms\n");
            printf("s: send mex (update var, command)\n");
            break;
    }
    return EXIT_SUCCESS;
}

static int send_mex(CanMessage *mex){
    return 0;
}

static int check_input_mex(void* args){
    struct can_frame* frame;
    CanMessage mex;
    while (1) {
        if(can_recv_frame(socket_can, frame)){
            continue;
        }
        mex.id = frame->can_id;
        mex.dlc = frame->can_dlc;
        memcpy(mex.rawMex.raw_buffer, frame->data, frame->can_dlc);
        dps_master_check_mex_recv(&mex);
    }
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

    thrd_t in_mex = 0;
    thrd_create(&in_mex, check_input_mex, NULL);

    
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
