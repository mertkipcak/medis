#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../hashmap/hashmap.h"
#include "../types/redis_types.h"

#define MAX_CLIENTS 10000
#define BUFFER_SIZE 4096
#define MAX_ARGS 100

// Server configuration
typedef struct {
    char* host;
    uint16_t port;
    int max_clients;
    bool daemonize;
} ServerConfig;

// Client connection structure
typedef struct {
    int fd;
    char* buffer;
    size_t buffer_size;
    size_t buffer_pos;
    bool authenticated;
} Client;

// Server structure
typedef struct {
    ServerConfig config;
    int server_fd;
    Hashmap* db;
    Client** clients;
    size_t client_count;
    bool running;
} Server;

// Function declarations
Server* server_create(const char* host, uint16_t port, int max_clients);
void server_destroy(Server* server);
bool server_start(Server* server);
void server_stop(Server* server);
bool server_is_running(const Server* server);

// Command handlers
bool handle_string_command(Server* server, Client* client, const char* command, char** args, int argc);
bool handle_list_command(Server* server, Client* client, const char* command, char** args, int argc);
bool handle_set_command(Server* server, Client* client, const char* command, char** args, int argc);
bool handle_sorted_set_command(Server* server, Client* client, const char* command, char** args, int argc);
bool handle_hash_command(Server* server, Client* client, const char* command, char** args, int argc);
bool handle_bitmap_command(Server* server, Client* client, const char* command, char** args, int argc);
bool handle_hyperloglog_command(Server* server, Client* client, const char* command, char** args, int argc);
bool handle_geo_command(Server* server, Client* client, const char* command, char** args, int argc);
bool handle_stream_command(Server* server, Client* client, const char* command, char** args, int argc);

// Response helpers
void send_ok(Client* client);
void send_error(Client* client, const char* error);
void send_integer(Client* client, int64_t value);
void send_string(Client* client, const char* str);
void send_array(Client* client, size_t size);
void send_null(Client* client);

#endif // SERVER_H 