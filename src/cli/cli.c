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

union GenericVal
{
  uint64_t u64;
  int64_t i64;
  float f32;
  double f64;
};
static void _print_var_value(const enum DATA_GENERIC_TYPE type, const uint8_t size, const union GenericVal val)
{
  switch (type)
  {
    case DATA_UNSIGNED:
      printf("%lu\n", val.u64);
      break;
    case DATA_SIGNED:
      printf("%ld\n", val.i64);
      break;
    case DATA_FLOATED:
      if (size==sizeof(float))
      {
        printf("%f\n", val.f32);
      }else
      {
        printf("%f\n", val.f64);
      }
      break;
  }
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
      uint8_t size = (uint8_t) (1u<< vars->vars[i].size);
      printf(
          "var name: %s, var id: %d, size: %d, type: %d, value: ",
          vars->vars[i].name, i,
          size,
          vars->vars[i].type);

      if (vars->vars[i].type == DATA_FLOATED)
      {
        if (size==sizeof(float)) {
          float d = vars->vars[i].v_f32;
          printf("%f\n", d);
        }else{
          double d = vars->vars[i].v_f64;
          printf("%lf\n", d);
        }
      } else {
        printf("%d\n", vars->vars[i].v_u32);
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
  int8_t err=0;

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
      VarRecord var = {0};
      if (dps_master_get_value_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id, &var)<0)
      {
        printf("variable not found %d\n", var_id);
        c1 = 0;
      }
      while (c1) {
        union GenericVal value ={0};
        const uint8_t size =(uint8_t) (1 << var.size);

        getchar();
        printf("u: update var\n");
        printf("f: fetch var value\n");
        printf("b: back\n");
        buffer = getchar();

        switch (buffer) {
        case 'u':
          printf("insert the new value[max 1024]: ");
          fflush(stdin);

          switch (var.type) {
          case DATA_UNSIGNED:
            scanf("%lu", &value.u64);
            break;
          case DATA_SIGNED:
            scanf("%ld", &value.i64);
            break;
          case DATA_FLOATED:
            if (size == sizeof(float)) {
              scanf("%f", &value.f32);
            }else{
              scanf("%lf", &value.f64);
            }
            break;
          }

          fflush(stdin);
          fflush(stdout);

          printf("sending b: %d\n",(uint8_t)board_id);
          printf("sending v: %d\n",(uint8_t)var_id);
          printf("sending val:");
          _print_var_value(var.type, size, value);

          if((err=dps_master_update_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id, &value, size))<0)
          {
            printf("err sending udpate req: %d\n",err);
          }
          dps_master_refresh_value_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id);

          break;
        case 'f':
          dps_master_refresh_value_var(&self->m_master, (uint8_t) board_id, (uint8_t) var_id);
          sleep(2);
          dps_master_get_value_var(&self->m_master,(uint8_t) board_id, (uint8_t) var_id, &var);
          printf("%s = ", var.name);
          _print_var_value(var.type, size, (union GenericVal){.u64=var.v_u64});
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
  getchar();
  return 0;
}

static int8_t send_mex(const DpsCanMessage* const restrict mex) {
  int8_t err=0;
  struct can_frame frame = {
      .can_id = mex->id,
      .can_dlc = mex->dlc,
  };

  memcpy(frame.data, &mex->full_word, mex->dlc);

  err= can_send_frame(SOCKET_CAN, &frame);

  sleep(2);

  return err;
}

static void wait_fun(void){
  sleep(1);
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
int8_t cli_init(Cli_h* const self, const char* const restrict can_node,
    const uint16_t master_id, const uint16_t slaves_id)
{
  union Cli_h_t_conv conv = {self};
  struct Cli_t* const p_self = conv.clear;

  memset(p_self, 0, sizeof(*p_self));

  SOCKET_CAN = can_init(can_node);
  if (SOCKET_CAN < 0)
  {
    fprintf(stderr, "failed init can interface: %s\n", can_node);
    return -1;
  }
  printf("using can node: %s\n", can_node);
  printf("using master id: %d\n", master_id);
  printf("using slaves id: %d\n", slaves_id);

  if (dps_master_init(&p_self->m_master, master_id, slaves_id, send_mex, wait_fun))
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
