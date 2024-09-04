#include "cli.h"
#include "../can_lib/canlib.h"
#include "./../../lib/DPS/dps_master.h"
#include <linux/can.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

// private

static int socket_can = 0;

static int get_board_input() {
  int board_id = -1;
  printf("board id [digit]: ");
  fflush(stdin);
  fflush(stdout);
  fscanf(stdin, "%d", &board_id);
  fflush(stdin);
  fflush(stdout);

  return board_id;
}

static int get_var_input() {
  int var_id = -1;
  printf("var id [digit]: ");
  fflush(stdin);
  fflush(stdout);
  fscanf(stdin, "%d", &var_id);
  fflush(stdin);
  fflush(stdout);

  return var_id;
}

static int get_com_input() {
  int var_id = -1;
  printf("com id [digit]: ");
  fflush(stdin);
  fflush(stdout);
  fscanf(stdin, "%d", &var_id);
  fflush(stdin);
  fflush(stdout);

  return var_id;
}

enum PRINT_INFO {
  P_BOARD,
  P_VAR,
  P_COMMAND,
};
static int print_info(enum PRINT_INFO type) {
  if (type == P_BOARD) {
  }
  board_list_info *boards = NULL;
  var_list_info *vars = NULL;
  com_list_info *coms = NULL;
  int8_t board_id = -1;

  switch (type) {
  case P_BOARD:
    boards = dps_master_list_board();
    for (uint8_t i = 0; i < boards->board_num; i++) {
      printf("board name: %s, board id: %d\n", boards->boards[i].name,
             boards->boards[i].id);
    }
    free(boards);
    return EXIT_SUCCESS;
    break;
  case P_VAR:
    board_id = get_board_input();
    if (board_id < 0) {
      return -1;
    }
    if (dps_master_list_vars(board_id, &vars)) {
      return -1;
    }
    for (uint8_t i = 0; i < vars->board_num; i++) {
      printf(
          "var name: %s, var id: %d, size: %d, floated: %d,signed :%d,value: ",
          vars->vars[i].name, vars->vars[i].metadata.full_data.obj_id.data_id,
          vars->vars[i].metadata.full_data.size,
          vars->vars[i].metadata.full_data.float_num,
          vars->vars[i].metadata.full_data.signe_num);
      if (vars->vars[i].metadata.full_data.float_num) {
        float *d = (float *)vars->vars[i].value;
        printf("%f\n", *d);
      } else {
        printf("%d\n", (int)vars->vars[i].value[0]);
      }
    }
    free(vars);
    vars = NULL;
    break;
  case P_COMMAND:
    if (dps_master_list_coms(&coms)) {
      return -1;
    }
    for (uint8_t i = 0; i < coms->board_num; i++) {
      printf("com name: %s, com id: %d,size: %d, floated: %d,signed :%d\n",
             coms->coms[i].name, coms->coms[i].metadata.full_data.com_id,
             coms->coms[i].metadata.full_data.size,
             coms->coms[i].metadata.full_data.float_num,
             coms->coms[i].metadata.full_data.signe_num);
    }
    free(coms);
    coms = NULL;
    break;
  default:
    break;
  }

  return 0;
}

static int send_req_slave() {
  char buffer = ' ';
  int8_t board_id = -1;
  int8_t var_id = -1;
  uint8_t c = 1;

  while (c) {
    getchar();
    printf("v: var category\n");
    printf("c: command category\n");
    printf("b: back\n");
    buffer = getchar();
    fflush(stdin);
    fflush(stdout);
    switch (buffer) {
    case 'v':
      board_id = get_board_input();
      if (board_id < 0) {
        printf("invalid board id\n");
        break;
      }

      var_id = get_var_input();
      if (var_id < 0) {
        printf("invalid var id\n");
        break;
      }
      uint8_t c1 = 1;
      var_record var = {};
      com_info com = {};
      if (dps_master_get_value_var(board_id, var_id, &var)) {
        printf("variable not found %d\n", var_id);
        c1 = 0;
      }
      while (c1) {
        getchar();
        printf("u: update var\n");
        printf("f: fetch var value\n");
        printf("b: back\n");
        buffer = getchar();
        char value[1024] = {};
        switch (buffer) {
        case 'u':
          printf("insert the new value[max 1024]: ");
          fflush(stdin);

          if (var.metadata.full_data.float_num) {
            scanf("%f", (float *)value);
          } else {
            scanf("%d", (int *)value);
          }
          fflush(stdin);
          fflush(stdout);
          dps_master_update_var(board_id, var_id, value,
                                var.metadata.full_data.size);
          dps_master_refresh_value_var(board_id, var_id);
          break;
        case 'f':
          dps_master_get_value_var(board_id, var_id, &var);
          printf("%s = ", var.name);
          if (var.metadata.full_data.float_num) {
            float *d = (float *)var.value;
            printf("%f\n", *d);
          } else {
            printf("%d\n", (int)var.value[0]);
          }
          break;
        case 'b':
          c1 = 0;
          break;
        }
      }
      break;
    case 'c':
      c1 = 1;
      var_id = get_com_input();
      if (dps_master_get_command_info(var_id, &com)) {
        printf("command not found\n");
        c1 = 0;
      }
      if (var_id < 0) {
        printf("invalid com id\n");
        break;
      }
      while (c1) {
        getchar();
        printf("f: write the full payload in decimal\n");
        printf("s: write the single bytes one by one\n");
        printf("b: back\n");
        buffer = getchar();
        char value[1024] = {};
        switch (buffer) {
        case 'f':
          printf("insert the new value[max 1024]: ");
          fflush(stdout);
          fflush(stdin);
          scanf("%d", (int *)value);
          fflush(stdin);
          fflush(stdout);
          if (dps_master_send_command(var_id, value,
                                      com.metadata.full_data.size)) {
            printf("failed send command\n");
          }
          break;
        case 's':
          for (int i = 0; i < com.metadata.full_data.size; i++) {
            getchar();
            printf("insert the byte in pos %d: ", i);
            fflush(stdout);
            fflush(stdin);
            scanf("%c", &value[i]);
            fflush(stdin);
            fflush(stdout);
          }
          if (dps_master_send_command(var_id, value,
                                      com.metadata.full_data.size)) {
            printf("failed send command\n");
          }
          break;
        case 'b':
          c1 = 0;
          break;
        }
      }
      break;
    case 'b':
      c = 0;
      break;
    }
  }
  return 0;
}

static int parser() {
  char buffer = ' ';
  int8_t board_id = -1;
  getchar();
  printf("(press h for help): ");
  fflush(stdin);
  fflush(stdout);
  buffer = getchar();
  fflush(stdin);
  fflush(stdout);
  switch (buffer) {
  case 'n':
    printf("new connections\n");
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
    printf("request info board: %d\n", board_id);
    if (dps_master_request_info_board(board_id, REQ_VAR | REQ_COM)) {
      printf("failed fetching data from board %d\n", board_id);
      return -1;
    }
    break;
  case 'b':
    printf("list boards\n");
    if (print_info(P_BOARD)) {
      return -1;
    }
    break;
  case 'v':
    printf("list vars\n");
    if (print_info(P_VAR)) {
      return -1;
    }
    break;
  case 'c':
    printf("list coms\n");
    if (print_info(P_COMMAND)) {
      return -1;
    }
    break;
  case 's':
    printf("send\n");
    if (send_req_slave()) {
      return -1;
    }
    break;
  case 'q':
    printf("quit\n");
    return 1;
  case 'h':
    printf("n: scan boards\n");
    printf("u: update info boards (one or all)\n");
    printf("b: list boards\n");
    printf("v: list vars\n");
    printf("c: list coms\n");
    printf("s: send mex (update var, command)\n");
    printf("q: quit\n");
    break;
  default:
    break;
  }
  return EXIT_SUCCESS;
}

static int send_mex(CanMessage *mex) {
  struct can_frame frame = {
      .can_id = mex->id,
      .can_dlc = mex->dlc,
  };

  memcpy(frame.data, mex->rawMex.raw_buffer, mex->dlc);
  // fprintf(stderr, "sending mex: id: %d, dlc: %d, data: ",
  // frame.can_id,frame.can_dlc); for (int i =0; i<frame.can_dlc; i++) {
  //     fprintf(stderr, "%d,", frame.data[i]);
  // }
  // fprintf(stderr, "\n");
  return can_send_frame(socket_can, &frame);
}

static int check_input_mex(void *args) {
  struct can_frame frame = {};
  CanMessage mex;
  while (1) {
    if (can_recv_frame(socket_can, &frame)) {
      continue;
    }
    mex.id = frame.can_id;
    mex.dlc = frame.can_dlc;
    memcpy(mex.rawMex.raw_buffer, frame.data, frame.can_dlc);

    // fprintf(stderr, "received mex: id: %d, dlc: %d, data: ",
    // frame.can_id,frame.can_dlc); for (int i =0; i<frame.can_dlc; i++) {
    //     fprintf(stderr, "%d,", frame.data[i]);
    // }
    // fprintf(stderr, "\n");
    dps_master_check_mex_recv(&mex);
  }

  return EXIT_SUCCESS;
}

// public
int cli_init() {
  char can_interface[1024] = {};
  printf("insert the name of the interface [max 1024]:");
  fflush(stdin);
  fscanf(stdin, "%s", can_interface);
  fflush(stdin);
  socket_can = can_init(can_interface);
  if (socket_can < 0) {
    fprintf(stderr, "failed init can interface: %s\n", can_interface);
    return EXIT_FAILURE;
  }
  if (dps_master_init(send_mex)) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
int cli_start() {
  thrd_t in_mex = 0;
  thrd_create(&in_mex, check_input_mex, NULL);

  while (1) {
    int out = parser();
    if (out > 0) {
      break;
    } else if (out < 0) {
      goto crush;
    }
  }

  return EXIT_SUCCESS;

crush:
  return EXIT_FAILURE;
}
