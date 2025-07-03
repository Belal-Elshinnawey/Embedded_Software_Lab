#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdint.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "udp_server.h"

#ifdef DEBUG_VIDEO // Enable UDP server only if DEBUG_VIDEO is defined
#define CHUNK_DATA_SIZE 1392  // 1400 - 8 bytes header
// Blocks until it receives at least one message, returns handler with client info
void udp_server_create(UDPServerHandler *server_handler, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[1]; // Dummy buffer to block on recv

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(sockfd);
        return;
    }

    printf("Server Created at Port %d\n", port);
    // Block until a message is received
    recvfrom(sockfd, buffer, sizeof(buffer), 0,
             (struct sockaddr *)&client_addr, &addr_len);

    server_handler->sockfd = sockfd;
    memcpy(&server_handler->client_addr, &client_addr, sizeof(client_addr));
    server_handler->addr_len = addr_len;
}

void udp_server_send(UDPServerHandler *handler, const void *data, size_t length)
{
    if (!handler)
    {
        printf("Failed to send to UDP Client \n");
        return;
    };
    sendto(handler->sockfd, data, length, 0,
                         (const struct sockaddr *)&handler->client_addr, handler->addr_len);
}




void udp_server_send_large(UDPServerHandler *handler, const void *data, size_t total_size) {
    if (!handler) return;

    static uint32_t frame_id = 0;
    const uint8_t *bytes = (const uint8_t *)data;

    uint16_t total_chunks = (total_size + CHUNK_DATA_SIZE - 1) / CHUNK_DATA_SIZE;

    for (uint16_t chunk_id = 0; chunk_id < total_chunks; chunk_id++) {
        uint8_t packet[1400];

        // Header
        memcpy(packet, &frame_id, 4);
        memcpy(packet + 4, &chunk_id, 2);
        memcpy(packet + 6, &total_chunks, 2);

        // Data
        size_t offset = chunk_id * CHUNK_DATA_SIZE;
        size_t chunk_size = (offset + CHUNK_DATA_SIZE > total_size)
                            ? total_size - offset
                            : CHUNK_DATA_SIZE;

        memcpy(packet + 8, bytes + offset, chunk_size);

        sendto(handler->sockfd, packet, chunk_size + 8, 0,
               (const struct sockaddr *)&handler->client_addr, handler->addr_len);
    }

    frame_id++;
}


#endif
