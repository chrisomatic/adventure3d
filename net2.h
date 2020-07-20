#pragma once

typedef enum
{
    PACKET_TYPE_NONE,
    PACKET_TYPE_NEW_PLAYER,
    PACKET_TYPE_PLAYER_UPDATE,
} PacketType;

typedef struct
{
    PacketType type;
    int client_id;
    float x,y,z; // position
} Packet;

int net_server_start();

void net_client_init();
void net_client_send(Packet* pkt);
int  net_client_recv(Packet* pkt);
void net_client_deinit();
