#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <errno.h> 

#include "util.h"
#include "math3d.h"
#include "net.h"
  
#define MAX_PACKET_SIZE 4096 
#define PORT    8888 

char server_ip_address[16] = {0};

typedef struct
{
    int socket;
    struct sockaddr_in client_addr;
    Vector3f position;
    float angle_h;
    float angle_v;
} Client;

static struct
{
    int tcp_socket;
    int udp_socket;
    struct sockaddr_in servaddr;
    int num_clients;
    Client clients[MAX_CLIENTS];
    u8 buffer[MAX_PACKET_SIZE];
} server_info;

static int net_client_get_id();

static void server_send_client_index(int client_index)
{
    int socket = server_info.clients[client_index].socket;
    printf("Sending client_index to socket: %d\n",socket);

    int res = send(socket, &client_index ,sizeof(int), 0);
    if(res < 0)
        perror("Send failed.\n");
    else
        printf("Sent.\n");
}

static void server_send_world_state(int client_index)
{
    ServerPacket srvpkt = {0};
    srvpkt.type = PACKET_TYPE_WORLD_STATE;
    srvpkt.num_clients = server_info.num_clients -1; // we don't want to send a client their own data

    int client_count = 0;
    for(int k = 0; k < MAX_CLIENTS; ++k)
    {
        if(server_info.clients[k].socket == 0) // ignore empty clients
            continue;

        if(server_info.clients[k].socket == server_info.clients[client_index].socket) // ignore client we are sending to
            continue;

        srvpkt.clients[client_count].position.x = server_info.clients[k].position.x;
        srvpkt.clients[client_count].position.y = server_info.clients[k].position.y;
        srvpkt.clients[client_count].position.z = server_info.clients[k].position.z;
        srvpkt.clients[client_count].angle_h    = server_info.clients[k].angle_h;
        srvpkt.clients[client_count].angle_v    = server_info.clients[k].angle_v;

        client_count++;
    }

    int size = sizeof(PacketType)+sizeof(u8)+(sizeof(ClientPacket)*srvpkt.num_clients) + 4;

    struct sockaddr_in addr = server_info.clients[client_index].client_addr;

    int len = sizeof(struct sockaddr_in);
    int res = sendto(server_info.udp_socket, &srvpkt, size, MSG_CONFIRM,
              (const struct sockaddr *)&addr,(socklen_t)len);

    if(res < 0)
        perror("Send failed.\n");
}

int net_server_start()
{
    memset(&server_info, 0, sizeof(server_info));

    // Creating socket file descriptor 
    server_info.tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_info.tcp_socket < 0)
    {
        perror("TCP Socket creation failed\n"); 
        return 1;
    } 

    printf("TCP Socket created.\n");

    server_info.udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_info.udp_socket < 0)
    {
        perror("UDP Socket creation failed\n"); 
        return 1;
    } 

    printf("UDP Socket created.\n");

    int optval = 1;
    if(setsockopt(server_info.tcp_socket, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int)))
    {
        perror("Failed to set socket options\n"); 
        return 1;
    } 
      
    server_info.servaddr.sin_family      = AF_INET; // IPv4
    server_info.servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_info.servaddr.sin_port        = htons(PORT);
      
    // Bind the socket with the server address 
    if (bind(server_info.tcp_socket, (const struct sockaddr *)&server_info.servaddr,sizeof(server_info.servaddr)) < 0)
    {
        perror("Bind failed"); 
        return 1;
    }

    printf("TCP Socket bind success.\n");
    
    if (bind(server_info.udp_socket, (const struct sockaddr *)&server_info.servaddr,sizeof(server_info.servaddr)) < 0)
    {
        perror("Bind failed"); 
        return 1;
    }

    printf("UDP Socket bind success.\n");

	//try to specify maximum of 3 pending connections for the master socket  
    if (listen(server_info.tcp_socket, 3) < 0)
    {
        perror("listen");
		return 1;
    }

    printf("Server started.\n");

    fd_set readfds;

	int addr_len = sizeof(struct sockaddr_in);

	for(;;)
	{
		//clear the socket set  
        FD_ZERO(&readfds);
        
        //add master socket to set  
        FD_SET(server_info.tcp_socket, &readfds);
        FD_SET(server_info.udp_socket, &readfds);

        int max_sd = server_info.tcp_socket;

        //add child sockets to set  
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            //socket descriptor  
            int sd = server_info.clients[i].socket;

            //if valid socket descriptor then add to read list  
            if(sd > 0)
                FD_SET(sd , &readfds);

            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        int activity = select(max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
            printf("select error");

        //If something happened on the server TCP socket,  
        //then its an incoming connection  
        if (FD_ISSET(server_info.tcp_socket, &readfds))
        {
            // get available client index
            int available_index = -1;
            for (int i = 0; i < MAX_CLIENTS; ++i)
            {
                //if position is empty  
                if( server_info.clients[i].socket == 0 )
                {
                    available_index = i;
                    break;
                }
            }

            if(available_index == -1)
            {
                printf("Client connection rejected. Server is full. Num Clients: %d\n",server_info.num_clients);
            }
            else 
            {
                int new_socket = accept(server_info.tcp_socket,(struct sockaddr *)&server_info.clients[available_index].client_addr, (socklen_t*)&addr_len);
                if (new_socket < 0)
                {
                    perror("accept");
                    return 1;
                }

                server_info.num_clients++;

                int addr_len = sizeof(struct sockaddr_in);
                getpeername(new_socket, (struct sockaddr*)&server_info.clients[available_index].client_addr,(socklen_t*)&addr_len);

                struct sockaddr_in* addr = &server_info.clients[available_index].client_addr;

                //inform user of socket number - used in send and receive commands  
                printf("Client connected, fd: %d, ip: %s, port: %d. Num Clients: %d\n",
                        new_socket,
                        inet_ntoa(addr->sin_addr),
                        ntohs(addr->sin_port),
                        server_info.num_clients
                );

                server_info.clients[available_index].socket = new_socket;
                printf("Adding to list of sockets as %d\n" , available_index);
                
                // send client index to client
                server_send_client_index(available_index);

                if(server_info.num_clients > 1)
                {
                    // send world state to new client
                    server_send_world_state(available_index);
                }
            }
        }

        if (FD_ISSET(server_info.udp_socket, &readfds))
        {
            struct sockaddr_in temp = {0};
            int len = sizeof(temp);
            int valread = recvfrom(server_info.udp_socket, server_info.buffer, MAX_PACKET_SIZE,
                          MSG_WAITALL, (struct sockaddr *)&temp, &len);

            if (valread > 0)
            {
                ClientPacket* pkt = (ClientPacket*)server_info.buffer;

                //printf("%d: P %f %f %f R %f %f\n",pkt->client_id, pkt->position.x,pkt->position.y,pkt->position.z, pkt->angle_h, pkt->angle_v);

                u8 client_index = pkt->client_id;

                server_info.clients[client_index].position.x = pkt->position.x;
                server_info.clients[client_index].position.y = pkt->position.y;
                server_info.clients[client_index].position.z = pkt->position.z;
                server_info.clients[client_index].angle_h = pkt->angle_h;
                server_info.clients[client_index].angle_v = pkt->angle_v;

                int size = sizeof(struct sockaddr_in);

                memcpy(&server_info.clients[client_index].client_addr,&temp,len);

                // broadcast new data to other clients
                if(server_info.num_clients > 1)
                {
                    for(int j = 0; j < MAX_CLIENTS; ++j)
                    {
                        if(server_info.clients[j].socket == 0) // ignore empty clients
                            continue;

                        //if(j == client_index) // ignore client that sent update
                        //    continue;

                        server_send_world_state(j);
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            int sd = server_info.clients[i].socket;

            if (FD_ISSET(sd , &readfds))
            {
                // Check if it was for closing , and also read the  
                // incoming message  
                int valread = read(sd, server_info.buffer, MAX_PACKET_SIZE);

                if (valread == 0)
                {
                    //Somebody disconnected , get their details and print  
                    struct sockaddr_in addr = {0};
                    getpeername(sd , (struct sockaddr*)&addr,(socklen_t*)&addr_len);
                    server_info.num_clients--;

                    printf("Client disconnected, ip: %s, port: %d. Num Clients: %d\n",
                          inet_ntoa(addr.sin_addr),
						  ntohs(addr.sin_port),
                          server_info.num_clients
                    );

                    //Close the socket and mark as 0 in list for reuse  
                    close(sd);
                    server_info.clients[i].socket = 0;

                    // broadcast data to other clients
                    for(int j = 0; j < MAX_CLIENTS; ++j)
                    {
                        if(server_info.clients[j].socket == 0) // ignore empty clients
                            continue;

                        server_send_world_state(j);
                    }
                }
            }
        }
	}

    return 0; 
}

static struct
{
    int tcp_socket;
    int udp_socket;
    struct sockaddr_in servaddr;
    u8 buffer[MAX_PACKET_SIZE];
} client_info;

int net_client_init()
{
    client_info.tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_info.tcp_socket < 0)
    {
        perror("TCP Socket creation failed"); 
        return -1;
    } 

    client_info.udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_info.udp_socket < 0)
    {
        perror("UDP Socket creation failed"); 
        return -1;
    } 

    memset(&client_info.servaddr, 0, sizeof(client_info.servaddr)); 
      
    // Filling server information 
    client_info.servaddr.sin_family = AF_INET;
    client_info.servaddr.sin_addr.s_addr = inet_addr(server_ip_address);
    client_info.servaddr.sin_port = htons(PORT); 

    // connect
    if (connect(client_info.tcp_socket, (struct sockaddr *)&client_info.servaddr , sizeof(client_info.servaddr)) < 0)
    {
        perror("Connect error");
        close(client_info.tcp_socket);
        return -1;
    }
    
    printf("Connected to %s.\n", server_ip_address);
    
    // get client id from server
    int client_id = net_client_get_id();

    if(client_id >= 0)
    {
        // record client id
        printf("Client id: %d\n",client_id);
    }

    return client_id;
}

void net_client_send(ClientPacket* pkt)
{
    int len = sizeof(client_info.servaddr);
    int res = sendto(client_info.udp_socket, pkt, sizeof(ClientPacket), MSG_CONFIRM,
              (const struct sockaddr *)&client_info.servaddr,(socklen_t)len);

    if(res < 0)
        perror("Send failed.\n");

    return;
}

static bool client_has_data_waiting(int socket, bool wait)
{
    fd_set readfds;

    //clear the socket set  
    FD_ZERO(&readfds);
    
    //add client socket to set  
    FD_SET(socket, &readfds);

    int activity;

    if(wait)
    {
        activity = select(socket + 1 , &readfds , NULL , NULL , NULL);
    }
    else
    {
        struct timeval tv = {0};
        activity = select(socket + 1 , &readfds , NULL , NULL , &tv);
    }

    if ((activity < 0) && (errno!=EINTR))
    {
        printf("select error");
        return false;
    }

    return FD_ISSET(socket , &readfds);
}

static int net_client_get_id()
{
    int id = -1;

    bool has_data = client_has_data_waiting(client_info.tcp_socket,true);

    if(has_data)
    {
        int valread = read(client_info.tcp_socket, client_info.buffer, MAX_PACKET_SIZE);

        if(valread > 0)
            id = client_info.buffer[0];
    }

    return id;
}

int net_client_recv(ServerPacket* pkt)
{
    int valread = 0;

    for(;;)
    {
        bool has_data = client_has_data_waiting(client_info.udp_socket,false);
        if(has_data)
        {
            int len = sizeof(client_info.servaddr);
            valread = recvfrom(client_info.udp_socket, (void*)client_info.buffer, MAX_PACKET_SIZE,
                      MSG_WAITALL, (struct sockaddr *) &client_info.servaddr,
                      &len);

            u8* bp = client_info.buffer;

            if(bp[0] == PACKET_TYPE_WORLD_STATE)
            {
                u8 num_clients = *(bp + sizeof(PacketType));
                memcpy(pkt,client_info.buffer,valread);
            }
        }
        else
        {
            break;
        }

    }

    return valread;
}

void net_client_deinit()
{
    close(client_info.tcp_socket); 
    close(client_info.udp_socket); 
}
