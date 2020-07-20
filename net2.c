#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <errno.h> 

#include "util.h"
#include "net.h"
  
#define MAX_CLIENTS 32
#define MAX_PACKET_SIZE 1024 
#define PORT    8888 

static struct
{
    int master_socket;
    struct sockaddr_in servaddr;
    struct sockaddr_in clients[MAX_CLIENTS];
    int num_clients;
    u8 buffer[MAX_PACKET_SIZE];
} server_info;

static int is_client(struct sockaddr_in address)
{
    for(int i = 0; i < server_info.num_clients; ++i)
        if(memcmp(&server_info.clients[i],&address, sizeof(struct sockaddr_in)) == 0)
            return i;

    return -1;
}

static int add_client(struct sockaddr_in address)
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server_info.clients[i].sin_addr.s_addr == 0)
        {
            memcpy(&server_info.clients[i],&address,sizeof(struct sockaddr_in));
            server_info.num_clients++;

            u32 ip = address.sin_addr.s_addr;

            printf("New Client! IP: %u.%u.%u.%u. Position: %d. Num Clients: %d\n",
                    (ip >> 24) && 0xFF,
                    (ip >> 16) && 0xFF,
                    (ip >>  8) && 0xFF,
                    (ip >>  0) && 0xFF,
                    i, server_info.num_clients);

            return i;
        }
    }

    return -1;
}

static int net_server_recv(Packet* pkt, int* client_id)
{
    struct sockaddr_in temp_addr;
    int len = sizeof(temp_addr);

    int n = recvfrom(server_info.master_socket, pkt, sizeof(Packet),  
                MSG_WAITALL, ( struct sockaddr *) &temp_addr, 
                &len); 

    int id = is_client(temp_addr);
    if(id < 0)
        id = add_client(temp_addr);

    *client_id = id;

    return n;
}

static void net_server_send(Packet* pkt, int client_id)
{
    sendto(server_info.master_socket, (const void*)pkt, sizeof(pkt),  
        MSG_CONFIRM, (const struct sockaddr *) &server_info.clients[client_id], 
        sizeof(server_info.clients[client_id])); 
}
  
int net_server_start()
{
    memset(&server_info, 0, sizeof(server_info));

    // Creating socket file descriptor 
    server_info.master_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_info.master_socket < 0)
    {
        perror("Socket creation failed\n"); 
        return 1;
    } 

    int optval = 1;
    if(setsockopt(server_info.master_socket, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int)))
    {
        perror("Failed to set socket options\n"); 
        return 1;
    } 
      
    server_info.servaddr.sin_family      = AF_INET; // IPv4
    server_info.servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_info.servaddr.sin_port        = htons(PORT);
      
    // Bind the socket with the server address 
    if (bind(server_info.master_socket, (const struct sockaddr *)&server_info.servaddr,sizeof(server_info.servaddr)) < 0)
    {
        perror("Bind failed"); 
        return 1;
    }
      
    Packet pkt = {0};

    for(;;)
    {
        int id = -1;
        int n = net_server_recv(&pkt, &id);

        if(n > 0)
        {
            //printf("id: %d, pos: %f %f %f\n",id,pkt.x, pkt.y, pkt.z);
            // send position update to all other clients
            for(int i = 0; i < server_info.num_clients; ++i)
            {
                if(i == id)
                    continue;

                net_server_send(&pkt,i);
            }
        }
    }

    return 0; 
}

static struct
{
    int sockfd;
    struct sockaddr_in servaddr;
    u8 buffer[MAX_PACKET_SIZE];
} client_info;

void net_client_init()
{
    // Creating socket file descriptor 
    if ((client_info.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed"); 
        return;
    } 
  
    memset(&client_info.servaddr, 0, sizeof(client_info.servaddr)); 
      
    // Filling server information 
    client_info.servaddr.sin_family = AF_INET; 
    client_info.servaddr.sin_port = htons(PORT); 
    client_info.servaddr.sin_addr.s_addr = INADDR_ANY; 
}

void net_client_send(Packet* pkt)
{
    sendto(client_info.sockfd, (const void *)pkt, sizeof(pkt), 
           MSG_CONFIRM, (const struct sockaddr *) &client_info.servaddr,  
           sizeof(client_info.servaddr)); 
}

int net_client_recv(Packet* pkt)
{
    int len; 
    int n = recvfrom(client_info.sockfd, (void *)client_info.buffer, MAX_PACKET_SIZE,  
                MSG_WAITALL, (struct sockaddr *) &client_info.servaddr, 
                &len); 

    printf("Received message from server. Num bytes: %d\n",n);
    return n;
}

void net_client_deinit()
{
    close(client_info.sockfd); 
}
