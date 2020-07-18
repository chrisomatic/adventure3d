#pragma once

typedef struct
{
    float x,y,z; // position
} Packet;

int net_server_start();

void net_client_init();
void net_client_send(Packet* pkt);
int  net_client_recv(Packet* pkt);
void net_client_deinit();
