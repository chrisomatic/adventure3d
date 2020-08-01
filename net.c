#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h> 
#include <time.h>

#include "socket.h"
#include "util.h"
#include "timer.h"
#include "net.h"
#include "packet_queue.h"

#define SERVER_RATE 30.0f
#define PORT 27001

#define PACKET_INFO_MAX_LEN 256
#define MAX_PRIOR_PACKETS 32
#define MAXIMUM_RTT 1.0f

#define DISCONNECTION_TIMEOUT 10.0f // seconds

typedef struct
{
    int socket;
    u16 local_latest_packet_id;
    u16 remote_latest_packet_id;
    PacketQueue latest_received_packets;
} NodeInfo;

typedef struct
{
    Address address;
    u16 remote_latest_packet_id;
    clock_t  time_of_latest_packet;
} ClientInfo;

typedef struct
{
    u16    packet_id;
    double time_sent;
} PacketInfo;

u32 game_id = 0;

// ---

static Address server_address = {0};
static ClientInfo server_clients[MAX_CLIENTS] = {0};

static u8 server_num_clients = 0;

static PacketInfo packet_info[PACKET_INFO_MAX_LEN] = {0};
static u32 packet_info_index = 0;
static Timer server_timer = {0};
static WorldState world_state = {0};

static void create_game_id()
{
    game_id = 0x98325423;
}

static inline int get_packet_size(Packet* pkt)
{
    return (sizeof(pkt->header) + pkt->data_len);
}

static u32 get_ack_bit_field(NodeInfo* node_info)
{
    u32 bit_field = 0;

    //node_info->latest_received_packets;

    return bit_field;
}

static inline bool is_packet_id_greater(u16 id, u16 cmp)
{
    return ((id > cmp) && (id - cmp <= 32768)) || 
           ((id < cmp) && (cmp - id  > 32768));
}

static void print_packet(Packet* pkt)
{
    printf("Game ID:      0x%08x\n",pkt->header.game_id);
    printf("Packet ID:    %u\n",pkt->header.packet_id);
    printf("Ack:          %u\n",pkt->header.ack);
    printf("Ack Bitfield: %u\n",pkt->header.ack_bitfield);
    printf("Data Len:     %u\n",pkt->data_len);
    printf("Data:\n");

    for(int i = 0; i < pkt->data_len; ++i)
    {
        printf("0x%02x ",pkt->data[i]);
    }
    printf("\n");
}

static bool has_data_waiting(int socket)
{
    fd_set readfds;

    //clear the socket set  
    FD_ZERO(&readfds);
    
    //add client socket to set  
    FD_SET(socket, &readfds);

    int activity;

    struct timeval tv = {0};
    activity = select(socket + 1 , &readfds , NULL , NULL , &tv);

    if ((activity < 0) && (errno!=EINTR))
    {
        perror("select error");
        return false;
    }

    bool has_data = FD_ISSET(socket , &readfds);
    return has_data;
}

static int net_send(NodeInfo* node_info, Address* to, Packet* pkt)
{
    int pkt_len = get_packet_size(pkt);
    int sent_bytes = socket_sendto(node_info->socket, to, (u8*)pkt, pkt_len);

    //printf("[SENT] Packet %d (%u B)\n",pkt->header.packet_id,sent_bytes);

    node_info->local_latest_packet_id++;

    return sent_bytes;
}

static int net_recv(NodeInfo* node_info, Address* from, Packet* pkt, bool* is_latest)
{
    int recv_bytes = socket_recvfrom(node_info->socket, from, (u8*)pkt);

    if(is_packet_id_greater(pkt->header.packet_id,node_info->remote_latest_packet_id))
    {
        node_info->remote_latest_packet_id = pkt->header.packet_id;
        *is_latest = true;
    }
    else
    {
        *is_latest = false;
    }

    //packet_queue_enqueue(&node_info->latest_received_packets,pkt);

    return recv_bytes;
}

static NodeInfo server_info = {0};

int net_server_start()
{
    int sock;

    printf("Creating socket.\n");
    socket_create(&sock);

    printf("Binding socket %u to any local ip on port %u.\n", sock, PORT);
    socket_bind(sock, NULL, PORT);

    server_info.socket = sock;

    printf("Creating packet queue.\n");
    /*
    bool queue_created = packet_queue_create(&server_info.latest_received_packets,MAX_PRIOR_PACKETS+1);
    if(!queue_created)
    {
        printf("Failed to create prior packets queue!\n");
        return -1;
    }
    */

    create_game_id();

    /*
    printf("Initializing GLFW.\n");

    if(!glfwInit())
    {
        fprintf(stderr,"Failed to init GLFW!\n");
        return false;
    }
    */

    printf("Server started.\n");

    timer_set_fps(&server_timer,SERVER_RATE);
    timer_begin(&server_timer);

    for(;;)
    {
        // Read packets
        for(;;)
        {
            bool data_waiting = has_data_waiting(server_info.socket);

            if(!data_waiting)
                break;

            Address from = {0};
            Packet recv_packet = {0};

            bool is_latest;
            int bytes_received = net_recv(&server_info, &from, &recv_packet, &is_latest);

            //print_packet(&recv_packet);

            // validate packet is legit
            if(recv_packet.header.game_id != game_id)
            {
                printf("Invalid packet game_id 0x%08x != 0x%08x\n",recv_packet.header.game_id, game_id);
                timer_delay_us(10); // delay 10 us
                continue;
            }

            bool new_client = true;
            int client_id = -1;

            for(int i = 0; i < server_num_clients; ++i)
            {
                // add client to client list if they are new
                if(server_clients[i].address.a    == from.a &&
                   server_clients[i].address.b    == from.b &&
                   server_clients[i].address.c    == from.c &&
                   server_clients[i].address.d    == from.d &&
                   server_clients[i].address.port == from.port)
                {
                    new_client = false;
                    client_id = i;
                    break;
                }
            }

            if(new_client)
            {
                client_id = server_num_clients;

                printf("Client Connected! %u.%u.%u.%u:%u\n",from.a,from.b,from.c,from.d,from.port);

                // Copy address to server clients addresses
                memcpy(&server_clients[server_num_clients].address,&from,sizeof(Address));
                server_num_clients++;
                world_state.num_clients = server_num_clients;

                printf("Num Clients: %u\n",server_num_clients);
            }

            is_latest = is_packet_id_greater(recv_packet.header.packet_id,server_clients[client_id].remote_latest_packet_id);

            if(new_client || is_latest)
            {
                server_clients[client_id].remote_latest_packet_id = recv_packet.header.packet_id;
                server_clients[client_id].time_of_latest_packet = timer_get_time();
                memcpy(&world_state.client_data[client_id],recv_packet.data,recv_packet.data_len);

                ClientData* c = (ClientData*)&world_state.client_data[client_id];
                printf("Client %u: P %f %f %f R %f %f\n",client_id,c->position.x,c->position.y,c->position.z,c->angle_h,c->angle_v);
            }

            timer_delay_us(10); // delay 10 us
        }

        if(server_num_clients > 0)
        {
            // disconnect any client that hasn't sent a packet in DISCONNECTION_TIMEOUT
            for(int i = 0; i < server_num_clients; ++i)
            {
                double time_elapsed = timer_get_time() - server_clients[i].time_of_latest_packet;

                if(time_elapsed >= DISCONNECTION_TIMEOUT)
                {
                    Address* addr = &server_clients[i].address;
                    printf("Client Disconnected! %u.%u.%u.%u:%u\n",addr->a,addr->b,addr->c,addr->d,addr->port);

                    // Disconnect client
                    server_num_clients--;
                    world_state.num_clients = server_num_clients;

                    memcpy(&server_clients[i], &server_clients[server_num_clients], sizeof(ClientInfo));

                    printf("Num Clients: %u\n",server_num_clients);
                }
            }
#if 1

            // Send packets to all "connected" clients
            for(int i = 0; i < server_num_clients; ++i)
            {
                // Fill out packet
                Packet send_packet = {
                    .header.game_id = game_id,
                    .header.packet_id = server_info.local_latest_packet_id,
                    .header.ack = server_clients[i].remote_latest_packet_id,
                    .header.ack_bitfield = get_ack_bit_field(&server_info)
                };

                world_state.ignore_id = i;
                send_packet.data_len = sizeof(world_state);
                memcpy(send_packet.data,(u8*)&world_state,sizeof(world_state));

                net_send(&server_info,&server_clients[i].address,&send_packet);
            }
#endif
            

            /*
            
            // Update packet info
            packet_info[packet_info_index].packet_id = send_packet.header.packet_id;
            packet_info[packet_info_index].time_sent = timer_get_elapsed(&server_timer);
            packet_info_index++;
            if(packet_info_index >= PACKET_INFO_MAX_LEN)
                packet_info_index = 0;
                */

            server_info.local_latest_packet_id++;
        }

        timer_wait_for_frame(&server_timer);
        timer_inc_frame(&server_timer);
    }
}

bool net_client_set_server_ip(char* address)
{
    // example input:
    // 200.100.24.10

    char num_str[3] = {0};
    u8   bytes[4]  = {0};

    u8   num_str_index = 0, byte_index = 0;

    for(int i = 0; i < strlen(address)+1; ++i)
    {
        if(address[i] == '.' || address[i] == '\0')
        {
            bytes[byte_index++] = atoi(num_str);
            memset(num_str,0,3*sizeof(char));
            num_str_index = 0;
            continue;
        }

        num_str[num_str_index++] = address[i];
    }

    server_address.a = bytes[0];
    server_address.b = bytes[1];
    server_address.c = bytes[2];
    server_address.d = bytes[3];

    server_address.port = PORT;

    return true;
}

static NodeInfo client_info = {0};

bool net_client_init()
{
    int sock;

    printf("Creating socket.\n");
    socket_create(&sock);

    client_info.socket = sock;
    create_game_id();

    return true;
}

bool net_client_data_waiting()
{
    bool data_waiting = has_data_waiting(client_info.socket);
    return data_waiting;
}

int net_client_send(u8* data, u32 len)
{
    Packet pkt = {
        .header.game_id = game_id,
        .header.packet_id = client_info.local_latest_packet_id,
        .header.ack = client_info.remote_latest_packet_id,
        .header.ack_bitfield = 0
    };

    memcpy(pkt.data,data,len);
    pkt.data_len = len;

    //print_packet(&pkt);

    int sent_bytes = net_send(&client_info, &server_address, &pkt);
    return sent_bytes;
}

int net_client_recv(Packet* pkt, bool* is_latest)
{
    Address from = {0};
    int recv_bytes = net_recv(&client_info, &from, pkt, is_latest);
    return recv_bytes;
}

void net_client_deinit()
{
    socket_close(client_info.socket);
}
