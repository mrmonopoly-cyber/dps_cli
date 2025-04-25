CC = gcc
C_FLAGS = -Wall -Wextra
C_EXFLAGS = 

cli_src_path := $(or $(CLI_ROOT), .)
cli.c = $(cli_src_path)/src/cli/cli.c
can_lib.c = $(cli_src_path)/src/can_lib/canlib.c

all: release

dps_master.c := ./lib/DPS/src/master/dps_master.c

DEBUG_FLAGS = -Werror -O0 -DDEBUG -fsanitize=undefine,address -g
RELEASE_FLAGS = -O2 -Werror
OBJ_LIST = dps_master.o c_vector.o dps_messages.o

debug: C_FLAGS += $(DEBUG_FLAGS)
debug: compile

release: C_FLAGS += $(RELEASE_FLAGS)
release: compile

compile: main

main: $(cli.c) $(OBJ_LIST)
	$(CC) $(C_FLAGS) $(C_EXFLAGS) $(DEBUG) $(OBJ_LIST) main.c $(cli.c) $(can_lib.c) -o main

cli_clean:
	rm -rf *.o
ifeq ($(wildcard main), main)
	rm main
endif

clean: cli_clean
