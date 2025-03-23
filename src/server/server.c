#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096
#define MAX_ARGS 64

// Forward declarations
static void handle_client(Server* server, Client* client);
static char** parse_command(const char* command, int* argc);
static void cleanup_client(Server* server, Client* client);

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

Server* server_create(const char* host, uint16_t port, int max_clients) {
    if (!host || port == 0 || max_clients <= 0) return NULL;
    
    Server* server = malloc(sizeof(Server));
    if (!server) return NULL;
    
    // Initialize server config
    server->config.host = strdup(host);
    if (!server->config.host) {
        free(server);
        return NULL;
    }
    server->config.port = port;
    server->config.max_clients = max_clients;
    server->config.daemonize = false;
    
    // Initialize server state
    server->server_fd = -1;
    server->db = hashmap_create();
    if (!server->db) {
        free(server->config.host);
        free(server);
        return NULL;
    }
    
    server->clients = calloc(max_clients, sizeof(Client*));
    if (!server->clients) {
        hashmap_destroy(server->db);
        free(server->config.host);
        free(server);
        return NULL;
    }
    
    server->client_count = 0;
    server->running = false;
    
    return server;
}

void server_destroy(Server* server) {
    if (!server) return;
    
    // Stop server if running
    if (server->running) {
        server_stop(server);
    }
    
    // Clean up all clients
    for (size_t i = 0; i < server->client_count; i++) {
        cleanup_client(server, server->clients[i]);
    }
    
    // Free client array
    free(server->clients);
    
    // Clean up database
    hashmap_destroy(server->db);
    
    // Free server config
    free(server->config.host);
    
    // Free server
    free(server);
}

bool server_start(Server* server) {
    if (!server) return false;
    
    // Create server socket
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        perror("Failed to create server socket");
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socket options");
        close(server->server_fd);
        return false;
    }
    
    // Set non-blocking mode
    int flags = fcntl(server->server_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(server->server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Failed to set non-blocking mode");
        close(server->server_fd);
        return false;
    }
    
    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server->config.host);
    addr.sin_port = htons(server->config.port);
    
    if (bind(server->server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind socket");
        close(server->server_fd);
        return false;
    }
    
    // Listen for connections
    if (listen(server->server_fd, server->config.max_clients) < 0) {
        perror("Failed to listen on socket");
        close(server->server_fd);
        return false;
    }
    
    server->running = true;
    printf("Server listening on %s:%d\n", server->config.host, server->config.port);
    
    // Main server loop
    while (server->running) {
        // Accept new connections
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server->server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd >= 0) {
            // Check if we can accept more clients
            if (server->client_count >= server->config.max_clients) {
                close(client_fd);
                continue;
            }
            
            // Set client socket to non-blocking mode
            int flags = fcntl(client_fd, F_GETFL, 0);
            if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
                perror("Failed to set client socket non-blocking");
                close(client_fd);
                continue;
            }
            
            // Create new client
            Client* client = malloc(sizeof(Client));
            if (!client) {
                close(client_fd);
                continue;
            }
            
            client->fd = client_fd;
            client->buffer = malloc(BUFFER_SIZE);
            if (!client->buffer) {
                free(client);
                close(client_fd);
                continue;
            }
            
            client->buffer_size = BUFFER_SIZE;
            client->buffer_pos = 0;
            client->authenticated = false;
            
            // Add client to array
            server->clients[server->client_count++] = client;
            printf("New client connected (%d/%d)\n", 
                   server->client_count, server->config.max_clients);
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Error accepting connection");
        }
        
        // Handle existing clients
        for (size_t i = 0; i < server->client_count; i++) {
            handle_client(server, server->clients[i]);
        }
        
        // Small delay to prevent busy waiting
        usleep(1000);
    }
    
    return true;
}

void server_stop(Server* server) {
    if (!server) return;
    server->running = false;
}

bool server_is_running(Server* server) {
    return server && server->running;
}

static void handle_client(Server* server, Client* client) {
    if (!server || !client) return;
    
    // Read data from client
    ssize_t n = read(client->fd, client->buffer + client->buffer_pos, 
                     client->buffer_size - client->buffer_pos);
    if (n <= 0) {
        if (n == 0) {
            // Client closed connection
            cleanup_client(server, client);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // Error reading from client
            perror("Error reading from client");
            cleanup_client(server, client);
        }
        return;
    }
    
    client->buffer_pos += n;
    
    // Process complete commands
    while (client->buffer_pos > 0) {
        // Find command end marker
        char* cmd_end = memchr(client->buffer, '\n', client->buffer_pos);
        if (!cmd_end) break;
        
        // Calculate command length
        size_t cmd_len = cmd_end - client->buffer;
        if (cmd_len > 0 && cmd_end[-1] == '\r') cmd_len--;
        
        // Create null-terminated command string
        char cmd[BUFFER_SIZE];
        if (cmd_len >= sizeof(cmd)) {
            send_error(client, "ERR command too long");
            cleanup_client(server, client);
            return;
        }
        memcpy(cmd, client->buffer, cmd_len);
        cmd[cmd_len] = '\0';
        
        // Parse command
        int argc;
        char** args = parse_command(cmd, &argc);
        if (!args) {
            send_error(client, "ERR out of memory");
            cleanup_client(server, client);
            return;
        }
        
        // Handle command
        if (argc > 0) {
            handle_command(server, client, args[0], args, argc);
        }
        
        // Free arguments
        for (int i = 0; i < argc; i++) {
            free(args[i]);
        }
        free(args);
        
        // Remove processed command from buffer
        size_t remaining = client->buffer_pos - (cmd_end - client->buffer + 1);
        if (remaining > 0) {
            memmove(client->buffer, cmd_end + 1, remaining);
        }
        client->buffer_pos = remaining;
    }
}

static char** parse_command(const char* command, int* argc) {
    char* cmd_copy = strdup(command);
    if (!cmd_copy) {
        *argc = 0;
        return NULL;
    }
    
    char** args = malloc(MAX_ARGS * sizeof(char*));
    if (!args) {
        free(cmd_copy);
        *argc = 0;
        return NULL;
    }
    
    *argc = 0;
    char* token = strtok(cmd_copy, " ");
    
    while (token && *argc < MAX_ARGS) {
        args[(*argc)++] = strdup(token);
        token = strtok(NULL, " ");
    }
    
    free(cmd_copy);
    return args;
}

static void cleanup_client(Server* server, Client* client) {
    if (!server || !client) return;
    
    // Remove client from array
    for (size_t i = 0; i < server->client_count; i++) {
        if (server->clients[i] == client) {
            // Shift remaining clients
            for (size_t j = i; j < server->client_count - 1; j++) {
                server->clients[j] = server->clients[j + 1];
            }
            server->client_count--;
            break;
        }
    }
    
    // Close socket
    close(client->fd);
    
    // Free client resources
    free(client->buffer);
    free(client);
}

bool handle_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 1) {
        send_error(client, "ERR invalid command");
        return false;
    }
    
    // Convert command to uppercase for comparison
    char cmd[32];
    strncpy(cmd, command, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    for (char* p = cmd; *p; p++) *p = toupper(*p);
    
    // Handle string commands
    if (strcmp(cmd, "SET") == 0 || strcmp(cmd, "GET") == 0) {
        return handle_string_command(server, client, command, args, argc);
    }
    // Handle list commands
    else if (strcmp(cmd, "LPUSH") == 0 || strcmp(cmd, "RPUSH") == 0 || strcmp(cmd, "LRANGE") == 0) {
        return handle_list_command(server, client, command, args, argc);
    }
    // Handle set commands
    else if (strcmp(cmd, "SADD") == 0 || strcmp(cmd, "SMEMBERS") == 0 || strcmp(cmd, "SISMEMBER") == 0) {
        return handle_set_command(server, client, command, args, argc);
    }
    // Handle sorted set commands
    else if (strcmp(cmd, "ZADD") == 0 || strcmp(cmd, "ZRANGE") == 0 || strcmp(cmd, "ZSCORE") == 0) {
        return handle_sorted_set_command(server, client, command, args, argc);
    }
    // Handle hash commands
    else if (strcmp(cmd, "HSET") == 0 || strcmp(cmd, "HGET") == 0 || strcmp(cmd, "HGETALL") == 0) {
        return handle_hash_command(server, client, command, args, argc);
    }
    // Handle bitmap commands
    else if (strcmp(cmd, "SETBIT") == 0 || strcmp(cmd, "GETBIT") == 0 || strcmp(cmd, "BITCOUNT") == 0) {
        return handle_bitmap_command(server, client, command, args, argc);
    }
    // Handle HyperLogLog commands
    else if (strcmp(cmd, "PFADD") == 0 || strcmp(cmd, "PFCOUNT") == 0 || strcmp(cmd, "PFMERGE") == 0) {
        return handle_hyperloglog_command(server, client, command, args, argc);
    }
    // Handle geo commands
    else if (strcmp(cmd, "GEOADD") == 0 || strcmp(cmd, "GEOPOS") == 0 || strcmp(cmd, "GEODIST") == 0) {
        return handle_geo_command(server, client, command, args, argc);
    }
    // Handle stream commands
    else if (strcmp(cmd, "XADD") == 0 || strcmp(cmd, "XRANGE") == 0 || strcmp(cmd, "XREAD") == 0) {
        return handle_stream_command(server, client, command, args, argc);
    }
    
    send_error(client, "ERR unknown command");
    return false;
}

// Response helper functions
void send_ok(Client* client) {
    if (!client) return;
    dprintf(client->fd, "+OK\r\n");
}

void send_error(Client* client, const char* error) {
    if (!client || !error) return;
    dprintf(client->fd, "-%s\r\n", error);
}

void send_integer(Client* client, int64_t value) {
    if (!client) return;
    dprintf(client->fd, ":%ld\r\n", value);
}

void send_string(Client* client, const char* str) {
    if (!client) return;
    if (!str) {
        send_null(client);
        return;
    }
    dprintf(client->fd, "$%zu\r\n%s\r\n", strlen(str), str);
}

void send_array(Client* client, size_t size) {
    if (!client) return;
    dprintf(client->fd, "*%zu\r\n", size);
}

void send_null(Client* client) {
    if (!client) return;
    dprintf(client->fd, "$-1\r\n");
} 