#ifndef UDP_SERVER_H
#define UDP_SERVER_H
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>  
#include <sys/types.h> 
#include <stddef.h> 

#ifdef DEBUG_VIDEO

typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
} UDPServerHandler;

void udp_server_create(UDPServerHandler * server_handler, int port);
void udp_server_send(UDPServerHandler *handler, const void *data, size_t length);
void udp_server_send_large(UDPServerHandler *handler, const void *data, size_t total_size);
#else
typedef struct {} UDPServerHandler;
static inline void udp_server_create(UDPServerHandler * server_handler, int port) {
    (void)port;
    (void)server_handler;
}
static inline void udp_server_send(UDPServerHandler *handler, const void *data, size_t length) {
    (void)handler;
    (void)data;
    (void)length;
    // No-op
}
static inline  void udp_server_send_large(UDPServerHandler *handler, const void *data, size_t total_size){
    (void)handler;
    (void)data;
    (void)total_size;
    // No-op
}
#endif  // DEBUG_VIDEO
#endif  // UDP_SERVER_H
