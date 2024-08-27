CC = gcc
C_FLAGS = -Wall -Wextra
C_EXFLAGS = 

cli_src_path := $(or $(CLI_ROOT), .)
cli.c = $(cli_src_path)/src/cli/cli.c
can_lib.c = $(cli_src_path)/src/can_lib/canlib.c

ifndef $(DPS_ROOT)
DPS_ROOT := $(cli_src_path)/lib/DPS
include $(DPS_ROOT)/Makefile
endif
all: release

DEBUG_FLAGS = -O0 -fsanitize=address -g
RELEASE_FLAGS = -O2
OBJ_LIST = c_check_input.o c_vector.o dps_master.o dps_slave.o

debug: C_FLAGS += $(DEBUG_FLAGS)
debug: compile

release: C_FLAGS += $(RELEASE_FLAGS)
release: compile


compile: main

main: $(cli.c) 
	$(CC) $(C_FLAGS) $(C_EXFLAGS) $(DEBUG) $(OBJ_LIST) main.c $(cli.c) $(can_lib.c) -o main

cli_clean:
ifeq ($(wildcard main), main)
	rm main
endif

clean: cli_clean
