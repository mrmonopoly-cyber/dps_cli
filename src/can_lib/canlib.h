#ifndef CANLIB_H
#define CANLIB_H

#include <linux/can.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Function prototypes
int8_t can_init(const char *ifname);
int8_t can_send_frame(int socket, struct can_frame *frame);
int8_t can_recv_frame(int socket, struct can_frame *frame);

#endif // CANLIB_H

