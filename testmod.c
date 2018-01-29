/*
 * Copyright © 2009-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <modbus.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define NB_CONNECTION    5

#if 1
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#endif


modbus_t *ctx = NULL;
int server_socket;
modbus_mapping_t *mb_mapping=NULL;


#if 1
#define  BUFF_SIZE   1024

typedef struct {
   long  data_type;
   int   data_num;
   unsigned char node_addr[4];
   unsigned char temperature[4];
   unsigned char humidity[4];
} t_data;

#endif

static void close_sigint(int dummy)
{
    close(server_socket);
    modbus_free(ctx);
    modbus_mapping_free(mb_mapping);

    exit(dummy);
}

#if 1
void *get_data_function(void* arg)
{
   int      msqid;
   t_data   data;

   if ( -1 == ( msqid = msgget( (key_t)9127, IPC_CREAT | 0666)))
   {
      perror( "msgget() 실패");
      exit( 1);
   }

   while( 1 )
   {
      // 메시지 큐 중에 data_type 이 2 인 자료만 수신
      if ( -1 == msgrcv( msqid, &data, sizeof( t_data) - sizeof( long), 0, 0))
      {
         perror( "msgrcv() 실패");
         exit( 1);
      }
      printf( "%d - node_addr[%02x%02x%02x%02x] \n", data.data_num, data.node_addr[0],data.node_addr[1],data.node_addr[2],data.node_addr[3]);
      printf( "temperature[%02x%02x%02x%02x] \n", data.temperature[0], data.temperature[1], data.temperature[2], data.temperature[3]);
      printf( "humidity[%02x%02x%02x%02x] \n", data.humidity[0], data.humidity[1], data.humidity[2], data.humidity[3]);

#if 1
//nodeid 4byte
//temperature 4byte
//humidity 4byte
//12byte
// 12/2 = 6
// 6ea

// TODO
// 1. if(new_node_id) 
//   list_count++;
//   add list
// 2. it can inform the total count
// 3. it can inform the complete status of network 
// map input register [0] => ready to inform the information
// map input register [1] => total router counter
// 
// if( needto check update count )
   if(mb_mapping != NULL)
   {
      there_is_same_thing = 0;
      for(i=0;i<total_node_count;i++)
      {
        //memcmp return value is that equal is zero
        if(memcmp(data.node_addr, &store_node_addr[i],4) == 0)
           there_is_same_thing = 1;
      }
      if(there_is_same_thing == 0)
      {
        memcpy(&store_node_addr[total_node_count], data.node_addr, 4);
        memcpy(&store_temperature[total_node_count], data.temperature, 4);
        memcpy(&store_humidity[total_node_count], data.humidity, 4);
      }
 
      mb_mapping->nb_input_registers
      mb_mapping->tap_input_registers[0];
      mb_mapping->tap_input_registers[6];
   }

#endif   

   }

}
#endif

int main(void)
{
    int master_socket;
    int rc;
    fd_set refset;
    fd_set rdset;
#if 1
    int thr_id;
    pthread_t p_thread;
#endif
    /* Maximum file descriptor number */
    int fdmax;


#if 1
    thr_id = pthread_create(&p_thread, NULL, get_data_function, NULL);
    if (thr_id < 0)
    {
        perror("thread create error : \n");
        exit(1);
    } 
#endif


    ctx = modbus_new_tcp("127.0.0.1", 1502);

    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0,
                                    MODBUS_MAX_READ_REGISTERS, 0);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    server_socket = modbus_tcp_listen(ctx, NB_CONNECTION);

    signal(SIGINT, close_sigint);

    /* Clear the reference set of socket */
    FD_ZERO(&refset);
    /* Add the server socket */
    FD_SET(server_socket, &refset);

    /* Keep track of the max file descriptor */
    fdmax = server_socket;

    for (;;) {
        rdset = refset;
        if (select(fdmax+1, &rdset, NULL, NULL, NULL) == -1) {
            perror("Server select() failure.");
            close_sigint(1);
        }

        /* Run through the existing connections looking for data to be
         * read */
        for (master_socket = 0; master_socket <= fdmax; master_socket++) {

            if (FD_ISSET(master_socket, &rdset)) {
                if (master_socket == server_socket) {
                    /* A client is asking a new connection */
                    socklen_t addrlen;
                    struct sockaddr_in clientaddr;
                    int newfd;

                    /* Handle new connections */
                    addrlen = sizeof(clientaddr);
                    memset(&clientaddr, 0, sizeof(clientaddr));
                    newfd = accept(server_socket, (struct sockaddr *)&clientaddr, &addrlen);
                    if (newfd == -1) {
                        perror("Server accept() error");
                    } else {
                        FD_SET(newfd, &refset);

                        if (newfd > fdmax) {
                            /* Keep track of the maximum */
                            fdmax = newfd;
                        }
                        printf("New connection from %s:%d on socket %d\n",
                               inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd);
                    }
                } else {
                    /* An already connected master has sent a new query */
                    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

                    modbus_set_socket(ctx, master_socket);
                    rc = modbus_receive(ctx, query);
                    if (rc != -1) {
                        modbus_reply(ctx, query, rc, mb_mapping);
                    } else {
                        /* Connection closed by the client, end of server */
                        printf("Connection closed on socket %d\n", master_socket);
                        close(master_socket);

                        /* Remove from reference set */
                        FD_CLR(master_socket, &refset);

                        if (master_socket == fdmax) {
                            fdmax--;
                        }
                    }
                }
            }
        }
    }

    return 0;
}
