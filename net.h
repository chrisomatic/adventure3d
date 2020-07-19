#pragma once

#define MAX_CLIENTS 32

typedef struct
{
    Vector3f position;
} ClientPacket;

typedef struct
{
    u8 num_clients;
    Vector3f client_positions[MAX_CLIENTS];
} ServerPacket;

int net_server_start();

void net_client_init();
void net_client_send(ClientPacket* pkt);
int  net_client_recv(ServerPacket* pkt);
void net_client_deinit();
