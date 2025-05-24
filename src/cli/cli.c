#include "cli.h"
#include "../can_lib/canlib.h"
#include "./../../lib/DPS/src/master/dps_master.h"
#include <linux/can.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <stdalign.h>

static int SOCKET_CAN;

struct Cli_t{
  DpsMaster_h m_master;
};

union Cli_h_t_conv {
  Cli_h* const restrict hidden;
  struct Cli_t* const restrict clear;
};

#ifdef DEBUG
char __assert_size_cli[sizeof(Cli_h)==sizeof(struct Cli_t)?1:-1];
char __assert_align_cli[alignof(Cli_h)==alignof(struct Cli_t)?1:-1];
#endif /* ifdef DEBUG */

// private

static int _get_board_input(void)
{
  int board_id = -1;
  printf("board id [digit]: ");
  fflush(stdin);
  fflush(stdout);
  fscanf(stdin, "%d", &board_id);
  fflush(stdin);
  fflush(stdout);

  return board_id;
}

static int _get_var_input(void)
{
  int var_id = -1;
  printf("var id [digit]: ");
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
static int _print_info(struct Cli_t* const restrict self, enum PRINT_INFO type) {
  if (type == P_BOARD) {
  }
  BoardListInfo *boards = NULL;
  VarListInfo *vars = NULL;
  int board_id = -1;

  switch (type) {
  case P_BOARD:
    boards = dps_master_list_board(&self->m_master);
    if (!boards)
    {
      break;
    }
    for (uint8_t i = 0; i < boards->board_num; i++) {
      printf("board name: %s, board id: %d\n", boards->boards[i].name,
             boards->boards[i].id);
    }
    free(boards);
    return EXIT_SUCCESS;
    break;
  case P_VAR:
    board_id = _get_board_input();
    if (board_id < 0)
    {
      return -1;
    }

    vars = dps_master_list_vars(&self->m_master, (uint8_t) board_id);

    if (!vars)
    {
      return -1;
    }

    for (uint8_t i = 0; i < vars->var_num; i++) {
      uint8_t size = 0;
      switch (vars->vars[i].size)
      {
        case 0:
          size = 1;
          break;
        case 1:
          size = 2;
          break;
        case 2:
          size = 4;
          break;
      }
      printf(
          "var name: %s, var id: %d, size: %d, type: %d, value: ",
          vars->vars[i].name, i,
          size,
          vars->vars[i].type);

      if (vars->vars[i].type == DATA_FLOATED)
      {
        float d = (float )vars->vars[i].value;
        printf("%f\n", d);
      } else {
        printf("%d\n", vars->vars[i].value);
      }
    }
    free(vars);
    vars = NULL;
    break;
  default:
    break;
  }

  return 0;
}

static int _send_req_slave(struct Cli_t* const restrict self)
{
  int buffer = ' ';
  int board_id = -1;
  int var_id = -1;
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
      board_id = _get_board_input();
      if (board_id < 0) {
        printf("invalid board id\n");
        break;
      }

      var_id = _get_var_input();
      if (var_id < 0) {
        printf("invalid var id\n");
        break;
      }
      uint8_t c1 = 1;
      VarRecord var = {};
      if (dps_master_get_value_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id, &var))
      {
        printf("variable not found %d\n", var_id);
        c1 = 0;
      }
      while (c1) {
        char value[1024] = {};
        uint8_t size =0;

        getchar();
        printf("u: update var\n");
        printf("f: fetch var value\n");
        printf("b: back\n");
        buffer = getchar();

        switch (buffer) {
        case 'u':
          printf("insert the new value[max 1024]: ");
          fflush(stdin);

          if (var.type == DATA_FLOATED)
          {
            scanf("%f", (float *)value);
          } else {
            scanf("%d", (int *)value);
          }
          fflush(stdin);
          fflush(stdout);

          switch (var.size)
          {
            case 0:
              size = 1;
              break;
            case 1:
              size = 2;
              break;
            case 2:
              size = 4;
          }

          dps_master_update_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id, value, size);
          dps_master_refresh_value_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id);

          break;
        case 'f':
          dps_master_update_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id, value, size);
          dps_master_refresh_value_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id);
          sleep(1);
          dps_master_get_value_var(&self->m_master,(uint8_t) board_id, (uint8_t) var_id, &var);
          printf("%s = ", var.name);
          if (var.type == DATA_FLOATED)
          {
            float d = (float )var.value;
            printf("%f\n", d);
          } else {
            printf("%d\n", var.value);
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

static int _parser(struct Cli_t* const restrict self)
{
  int buffer = ' ';
  int board_id = -1;
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
    if (dps_master_new_connection(&self->m_master)<0)
    {
      return -1;
    }
    break;
  case 'u':
    printf("update info of one board \n");
    board_id = _get_board_input();
    if (board_id < 0) {
      break;
    }
    printf("request info board: %d\n", board_id);
    if (dps_master_request_info_board(&self->m_master, (uint8_t) board_id, REQ_VAR)<0)
    {
      printf("failed fetching data from board %d\n", board_id);
      return -1;
    }
    break;
  case 'b':
    printf("list boards\n");
    if (_print_info(self, P_BOARD)<0)
    {
      return -1;
    }
    break;
  case 'v':
    printf("list vars\n");
    if (_print_info(self, P_VAR)<0)
    {
      return -1;
    }
    break;
  case 's':
    printf("send\n");
    if (_send_req_slave(self)<0)
    {
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
    printf("s: send mex (update var, command)\n");
    printf("q: quit\n");
    break;
  default:
    break;
  }
  return 0;
}

static int8_t send_mex(const DpsCanMessage* const restrict mex) {
  struct can_frame frame = {
      .can_id = mex->id,
      .can_dlc = mex->dlc,
  };

  printf("sendind mex with id: %d\n",mex->id);
  memcpy(frame.data, &mex->full_word, mex->dlc);

  return can_send_frame(SOCKET_CAN, &frame);
}

static int check_input_mex(void *args) {
  struct can_frame frame = {};
  DpsCanMessage mex;
  DpsMaster_h* master = args;

  while (1) {
    if (can_recv_frame(SOCKET_CAN, &frame)) {
      continue;
    }
    mex.id = (uint16_t) frame.can_id;
    mex.dlc = frame.can_dlc;
    memcpy(&mex.full_word, frame.data, frame.can_dlc);

    dps_master_check_mex_recv(master,&mex);
  }

  return EXIT_SUCCESS;
}

// public
int8_t cli_init(Cli_h* const self)
{
  union Cli_h_t_conv conv = {self};
  struct Cli_t* const p_self = conv.clear;

  memset(p_self, 0, sizeof(*p_self));

  char can_interface[1024] = {};
  uint32_t master_id=0;
  uint32_t slaves_id=0;

  printf("insert the name of the interface [max 1024]:");
  fflush(stdin);
  fscanf(stdin, "%s", can_interface);
  fflush(stdin);
  SOCKET_CAN = can_init(can_interface);
  if (SOCKET_CAN < 0)
  {
    fprintf(stderr, "failed init can interface: %s\n", can_interface);
    return -1;
  }
  printf("insert the id of the master:");
  fflush(stdin);
  fscanf(stdin, "%d", &master_id);
  fflush(stdin);

  printf("insert the id of the slaves:");
  fflush(stdin);
  fscanf(stdin, "%d", &slaves_id);
  fflush(stdin);

  printf("using master id: %d\n", master_id);
  printf("using slaves id: %d\n", slaves_id);

  if (dps_master_init(&p_self->m_master, (uint16_t) master_id, (uint16_t) slaves_id, send_mex))
  {
    return -2;
  }
  return 0;
}

int8_t cli_start(Cli_h* const self)
{
  union Cli_h_t_conv conv = {self};
  struct Cli_t* const p_self = conv.clear;
  thrd_t in_mex = 0;

  thrd_create(&in_mex, check_input_mex, &p_self->m_master);

  while (1) {
    int out = _parser(p_self);
    if (out > 0) {
      break;
    } else if (out < 0) {
      goto crush;
    }
  }

  return 0;

crush:
  return -1;
}
