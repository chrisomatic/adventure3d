#pragma once

#define MAX_CLIENTS 32

typedef struct
{
    u8 client_id;
    Vector3f position;
    float angle_h;
    float angle_v;
} ClientPacket;

typedef enum
{
    PACKET_TYPE_NONE,
    PACKET_TYPE_WORLD_STATE,
    PACKET_TYPE_MESSAGE,
} PacketType;

typedef struct
{
    PacketType type;
    u8 num_clients;
    ClientPacket clients[MAX_CLIENTS];
} ServerPacket;

extern char server_ip_address[16];

int net_server_start();

int net_client_init();
void net_client_send(ClientPacket* pkt);
int net_client_recv(ServerPacket* pkt);
void net_client_deinit();
