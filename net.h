#pragma once

#include "math3d.h"

#define MAX_CLIENTS 8
#define MAX_PACKET_DATA_SIZE 1024

typedef struct
{
    u32 game_id;
    u16 packet_id;
    u16 ack;
    u32 ack_bitfield;
} PacketHeader;

typedef struct
{
    PacketHeader header;

    u32 data_len;
    u8  data[MAX_PACKET_DATA_SIZE];
} Packet;

typedef struct
{
    Vector3f position;
    float angle_h;
    float angle_v;
    float junk; // @TODO: figure out why final 4 bytes don't get received!
} ClientData;

typedef struct
{
    u16        version;
    u16        num_clients;
    u16        ignore_id;
    ClientData client_data[MAX_CLIENTS];
} WorldState;

extern u32 game_id;
extern char* server_ip_address;

// Server
int net_server_start();

// Client
bool net_client_init();
bool net_client_set_server_ip(char* address);
bool net_client_data_waiting();
int net_client_send(u8* data, u32 len);
int net_client_recv(Packet* pkt, bool* is_latest);
void net_client_deinit();
