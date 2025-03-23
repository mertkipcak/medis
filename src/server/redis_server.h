#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../hashmap/hashmap.h"

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 6379

typedef struct {
    int socket;
    char buffer[BUFFER_SIZE];
    size_t buffer_pos;
    bool connected;
} Client;

typedef struct {
    int server_socket;
    Client* clients[MAX_CLIENTS];
    HashMap* db;
    bool running;
} RedisServer;

// Server management
RedisServer* createRedisServer(uint16_t port);
void startRedisServer(RedisServer* server);
void stopRedisServer(RedisServer* server);
void freeRedisServer(RedisServer* server);

// Client management
Client* createClient(int socket);
void freeClient(Client* client);
void handleClient(RedisServer* server, Client* client);

// Command handling
void processCommand(RedisServer* server, Client* client, const char* command);
char* formatResponse(const char* response);

#endif // REDIS_SERVER_H 