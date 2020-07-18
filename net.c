#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "util.h"
#include "net.h"
  
#define MAX_PACKET_SIZE 1024 
#define PORT    8080 

static struct
{
    int sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    u8 buffer[MAX_PACKET_SIZE];
} server_info;

static int net_server_recv(Packet* pkt)
{
    int len = sizeof(server_info.cliaddr);
    int n = recvfrom(server_info.sockfd, pkt, sizeof(Packet),  
                MSG_WAITALL, ( struct sockaddr *) &server_info.cliaddr, 
                &len); 
    return n;
}

static void net_server_send(Packet* pkt)
{
    int len = sizeof(server_info.cliaddr);
    sendto(server_info.sockfd, (const void*)pkt, sizeof(pkt),  
        MSG_CONFIRM, (const struct sockaddr *) &server_info.cliaddr, 
        len); 
}
  
int net_server_start()
{
    char buffer[MAX_PACKET_SIZE]; 

    // Creating socket file descriptor 
    if ((server_info.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed\n"); 
        return 1;
    } 
      
    memset(&server_info.servaddr, 0, sizeof(server_info.servaddr)); 
    memset(&server_info.cliaddr, 0, sizeof(server_info.cliaddr)); 
      
    // Filling server information 
    server_info.servaddr.sin_family    = AF_INET; // IPv4 
    server_info.servaddr.sin_addr.s_addr = INADDR_ANY; 
    server_info.servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if (bind(server_info.sockfd, (const struct sockaddr *)&server_info.servaddr,sizeof(server_info.servaddr)) < 0) 
    {
        perror("Bind failed"); 
        return 1;
    }
      
  
    Packet pkt = {0};
    for(;;)
    {
        int n = net_server_recv(&pkt);

        if(n > 0)
            printf("pos: %f %f %f\n",pkt.x, pkt.y, pkt.z);
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
