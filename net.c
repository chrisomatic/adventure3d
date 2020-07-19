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
#include "math3d.h"
#include "net.h"
  
#define MAX_PACKET_SIZE 1024 
#define PORT    8888 

typedef struct
{
    int socket;
    Vector3f position;
} Client;

static struct
{
    int tcp_socket;
    struct sockaddr_in servaddr;
    Client clients[MAX_CLIENTS];
    int num_clients;
    u8 buffer[MAX_PACKET_SIZE];
} server_info;

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

	//try to specify maximum of 3 pending connections for the master socket  
    if (listen(server_info.tcp_socket, 3) < 0)
    {
        perror("listen");
		return 1;
    }

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
        int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

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

            //send new connection greeting message  
			const char* message = "Welcome!";
            if(send(new_socket, message, strlen(message), 0) != strlen(message) )
            {
                perror("send");
            }

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
                }
                else
                {
					ClientPacket* pkt = (ClientPacket*)server_info.buffer;
					printf("%d: P %f %f %f\n",sd, pkt->position.x,pkt->position.y,pkt->position.z, valread);

                    // broadcast new data to other clients
                    if(server_info.num_clients > 1)
                    {
                        ServerPacket srvpkt = {0};

                        srvpkt.num_clients = server_info.num_clients;
                        int client_count = 0;
                        for(int i = 0; i < MAX_CLIENTS; ++i)
                        {
                            if(server_info.clients[i].socket == 0)
                                continue;
                            
                            srvpkt.client_positions[client_count].x = server_info.clients[i].position.x;
                            srvpkt.client_positions[client_count].y = server_info.clients[i].position.y;
                            srvpkt.client_positions[client_count].z = server_info.clients[i].position.z;

                            client_count++;
                        }

                        for(int i = 0; i < MAX_CLIENTS; ++i)
                        {
                            if(server_info.clients[i].socket == 0)
                                continue;

                            if(server_info.clients[i].socket == sd)
                                continue;

                            int res = send(server_info.clients[i].socket, &srvpkt ,sizeof(ServerPacket), 0);
                            if(res < 0)
                                perror("Send failed.\n");
                        }
                    }
                }
            }
        }
	}

#if 0
      
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
#endif

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
    client_info.servaddr.sin_port = htons(PORT); 
    client_info.servaddr.sin_addr.s_addr = INADDR_ANY; 

    // connect
    if (connect(client_info.sockfd, (struct sockaddr *)&client_info.servaddr , sizeof(client_info.servaddr)) < 0)
    {
        perror("Connect error");
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
    int res = recv(client_info.sockfd, client_info.buffer, MAX_PACKET_SIZE, 0);
    if( res < 0)
    {
		perror("recv failed");
        return 0;
    }

    memcpy(pkt,client_info.buffer,sizeof(ServerPacket));

    return res;
}

void net_client_deinit()
{
    close(client_info.sockfd); 
}
