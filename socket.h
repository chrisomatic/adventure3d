#pragma once

#include <stdbool.h>

#include "util.h"

typedef struct
{
    u8 a;
    u8 b;
    u8 c;
    u8 d;
    u16 port;
} Address;

bool socket_initalize();
void socket_shutdown();

bool socket_create(int* socket_handle);
bool socket_bind(int socket_handle, Address* address, u16 port);
void socket_close(int socket_handle);

int socket_sendto(int socket_handle, Address* address, u8* pkt, u32 pkt_size);
int socket_recvfrom(int socket_handle, Address* address, u8* pkt);
