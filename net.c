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
#include <errno.h> 

#include "util.h"
#include "math3d.h"
#include "net.h"
  
#define MAX_PACKET_SIZE 4096 
#define PORT    8888 

typedef struct
{
    int socket;
    Vector3f position;
    float angle_h;
    float angle_v;
} Client;

static struct
{
    int tcp_socket;
    struct sockaddr_in servaddr;
    int num_clients;
    Client clients[MAX_CLIENTS];
    u8 buffer[MAX_PACKET_SIZE];
} server_info;

static void server_send_world_state(int socket)
{
    ServerPacket srvpkt = {0};
    srvpkt.type = PACKET_TYPE_WORLD_STATE;
    srvpkt.num_clients = server_info.num_clients -1; // we don't want to send a client their own data

    int client_count = 0;
    for(int k = 0; k < MAX_CLIENTS; ++k)
    {
        if(server_info.clients[k].socket == 0) // ignore empty clients
            continue;

        if(server_info.clients[k].socket == socket) // ignore client we are sending to
            continue;

        srvpkt.clients[client_count].position.x = server_info.clients[k].position.x;
        srvpkt.clients[client_count].position.y = server_info.clients[k].position.y;
        srvpkt.clients[client_count].position.z = server_info.clients[k].position.z;
        srvpkt.clients[client_count].angle_h    = server_info.clients[k].angle_h;
        srvpkt.clients[client_count].angle_v    = server_info.clients[k].angle_v;

        client_count++;
    }


    int size = sizeof(PacketType)+sizeof(u8)+(sizeof(ClientPacket)*srvpkt.num_clients) + 4;
    int res = send(socket, &srvpkt ,size, 0);
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
        perror("Socket creation failed\n"); 
        return 1;
    } 

    printf("TCP Socket created.\n");

    int optval = 1;
    if(setsockopt(server_info.tcp_socket, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int)))
    {
        perror("Failed to set socket options\n"); 
        return 1;
    } 
      
    server_info.servaddr.sin_family      = AF_INET; // IPv4
    server_info.servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //server_info.servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.servaddr.sin_port        = htons(PORT);
      
    // Bind the socket with the server address 
    if (bind(server_info.tcp_socket, (const struct sockaddr *)&server_info.servaddr,sizeof(server_info.servaddr)) < 0)
    {
        perror("Bind failed"); 
        return 1;
    }

    printf("Socket bind success.\n");

	//try to specify maximum of 3 pending connections for the master socket  
    if (listen(server_info.tcp_socket, 3) < 0)
    {
        perror("listen");
		return 1;
    }

    printf("Server started.\n");

    fd_set readfds;

	int addrlen = sizeof(server_info.servaddr);

	for(;;)
	{
		//clear the socket set  
        FD_ZERO(&readfds);
        
        //add master socket to set  
        FD_SET(server_info.tcp_socket, &readfds);
        int max_sd = server_info.tcp_socket;

        //add child sockets to set  
        for (int i = 0 ; i < MAX_CLIENTS ; ++i)
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

        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(server_info.tcp_socket, &readfds))
        {
			int new_socket = accept(server_info.tcp_socket,(struct sockaddr *)&server_info.servaddr, (socklen_t*)&addrlen);
            if (new_socket < 0)
            {
                perror("accept");
                return 1;
            }

            server_info.num_clients++;

            //inform user of socket number - used in send and receive commands  
            printf("Client connected, fd: %d, ip: %s, port: %d. Num Clients: %d\n",
					new_socket,
					inet_ntoa(server_info.servaddr.sin_addr),
                    ntohs(server_info.servaddr.sin_port),
                    server_info.num_clients
            );

            //add new socket to array of sockets  
            for (int i = 0; i < MAX_CLIENTS; ++i)
            {
                //if position is empty  
                if( server_info.clients[i].socket == 0 )
                {
                    server_info.clients[i].socket = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);

                    break;
                }
            }

            if(server_info.num_clients > 1)
            {
                // send world state to new client
                server_send_world_state(new_socket);
            }
        }

        //else its some IO operation on some other socket 
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            int sd = server_info.clients[i].socket;

            if (FD_ISSET(sd , &readfds))
            {
                //Check if it was for closing , and also read the  
                //incoming message  
				int valread = read(sd, server_info.buffer, MAX_PACKET_SIZE);

                if (valread == 0)
                {
                    //Somebody disconnected , get his details and print  
                    getpeername(sd , (struct sockaddr*)&server_info.servaddr,(socklen_t*)&addrlen);
                    server_info.num_clients--;

                    printf("Client disconnected, ip: %s, port: %d. Num Clients: %d\n",
                          inet_ntoa(server_info.servaddr.sin_addr),
						  ntohs(server_info.servaddr.sin_port),
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

                        server_send_world_state(server_info.clients[j].socket);
                    }
                }
                else
                {
					ClientPacket* pkt = (ClientPacket*)server_info.buffer;
					printf("%d: P %f %f %f R %f %f\n",sd, pkt->position.x,pkt->position.y,pkt->position.z, pkt->angle_h, pkt->angle_v);

                    server_info.clients[i].position.x = pkt->position.x;
                    server_info.clients[i].position.y = pkt->position.y;
                    server_info.clients[i].position.z = pkt->position.z;
                    server_info.clients[i].angle_h = pkt->angle_h;
                    server_info.clients[i].angle_v = pkt->angle_v;

                    // broadcast new data to other clients
                    if(server_info.num_clients > 1)
                    {
                        for(int j = 0; j < MAX_CLIENTS; ++j)
                        {
                            if(server_info.clients[j].socket == 0) // ignore empty clients
                                continue;

                            if(server_info.clients[j].socket == sd) // ignore client that sent update
                                continue;

                            server_send_world_state(server_info.clients[j].socket);
                        }
                    }
                }
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
    if ((client_info.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed"); 
        return;
    } 

    memset(&client_info.servaddr, 0, sizeof(client_info.servaddr)); 
      
    // Filling server information 
    client_info.servaddr.sin_family = AF_INET;
    //client_info.servaddr.sin_addr.s_addr = INADDR_ANY;
    client_info.servaddr.sin_addr.s_addr = inet_addr("66.228.43.91");
    client_info.servaddr.sin_port = htons(PORT); 

    // connect
    if (connect(client_info.sockfd, (struct sockaddr *)&client_info.servaddr , sizeof(client_info.servaddr)) < 0)
    {
        perror("Connect error");
        close(client_info.sockfd);
        return;
    }
    
    printf("Connected.\n");

}

void net_client_send(ClientPacket* pkt)
{
    int res = send(client_info.sockfd, pkt ,sizeof(ClientPacket), 0);
    
    if(res < 0)
        perror("Send failed.\n");

    return;
}

int net_client_recv(ServerPacket* pkt)
{
    fd_set readfds;

    //clear the socket set  
    FD_ZERO(&readfds);
    
    //add client socket to set  
    FD_SET(client_info.sockfd, &readfds);

    int sd = client_info.sockfd;

    struct timeval tv = {0};
    int activity = select(sd + 1 , &readfds , NULL , NULL , &tv);
    if ((activity < 0) && (errno!=EINTR))
    {
        printf("select error");
        return 0;
    }

    int valread = 0;
    if(FD_ISSET(sd , &readfds))
    {
        valread = read(sd, client_info.buffer, MAX_PACKET_SIZE);
        if(valread > 0)
        {
            u8* bp = client_info.buffer;

            if(bp[0] == PACKET_TYPE_WORLD_STATE)
            {
                u8 num_clients = *(bp + sizeof(PacketType));
                memcpy(pkt,client_info.buffer,valread);
            }

        }
    }
    else
    {
        return 0;
    }

    return valread;
}

void net_client_deinit()
{
    close(client_info.sockfd); 
}
