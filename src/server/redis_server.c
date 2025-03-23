#include "redis_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

RedisServer* createRedisServer(uint16_t port) {
    RedisServer* server = malloc(sizeof(RedisServer));
    if (!server) return NULL;

    server->db = createHashMap();
    server->running = false;
    
    // Initialize client array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        server->clients[i] = NULL;
    }

    // Create server socket
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
        free(server);
        return NULL;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server->server_socket);
        free(server);
        return NULL;
    }

    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server->server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server->server_socket);
        free(server);
        return NULL;
    }

    return server;
}

Client* createClient(int socket) {
    Client* client = malloc(sizeof(Client));
    if (!client) return NULL;

    client->socket = socket;
    client->buffer_pos = 0;
    client->connected = true;
    memset(client->buffer, 0, BUFFER_SIZE);

    return client;
}

void freeClient(Client* client) {
    if (client) {
        close(client->socket);
        free(client);
    }
}

void freeRedisServer(RedisServer* server) {
    if (!server) return;

    // Free all clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i]) {
            freeClient(server->clients[i]);
            server->clients[i] = NULL;
        }
    }

    // Free database
    if (server->db) {
        freeHashMap(server->db);
    }

    // Close server socket
    if (server->server_socket >= 0) {
        close(server->server_socket);
    }

    free(server);
}

char* formatResponse(const char* response) {
    // Simple RESP formatting for now
    size_t len = strlen(response);
    char* resp = malloc(len + 3); // +3 for $, \r, \n
    if (!resp) return NULL;

    snprintf(resp, len + 3, "$%zu\r\n%s\r\n", len, response);
    return resp;
}

void processCommand(RedisServer* server, Client* client, const char* command) {
    // Simple command parsing for now
    char cmd[32];
    char key[256];
    char value[1024];
    
    if (sscanf(command, "%s %s %s", cmd, key, value) >= 1) {
        if (strcasecmp(cmd, "SET") == 0) {
            hashMapInsert(server->db, key, (unsigned char*)value, strlen(value) + 1);
            char* response = formatResponse("OK");
            send(client->socket, response, strlen(response), 0);
            free(response);
        }
        else if (strcasecmp(cmd, "GET") == 0) {
            size_t valueSize;
            unsigned char* value = hashMapGet(server->db, key, &valueSize);
            if (value) {
                char* response = formatResponse((char*)value);
                send(client->socket, response, strlen(response), 0);
                free(response);
            } else {
                char* response = formatResponse("(nil)");
                send(client->socket, response, strlen(response), 0);
                free(response);
            }
        }
        else if (strcasecmp(cmd, "DEL") == 0) {
            hashMapRemove(server->db, key);
            char* response = formatResponse("1");
            send(client->socket, response, strlen(response), 0);
            free(response);
        }
        else {
            char* response = formatResponse("ERR unknown command");
            send(client->socket, response, strlen(response), 0);
            free(response);
        }
    }
}

void handleClient(RedisServer* server, Client* client) {
    ssize_t bytes_read = recv(client->socket, client->buffer + client->buffer_pos, 
                            BUFFER_SIZE - client->buffer_pos, 0);
    
    if (bytes_read <= 0) {
        client->connected = false;
        return;
    }

    client->buffer_pos += bytes_read;
    
    // Process complete commands
    char* cmd_end = strstr(client->buffer, "\r\n");
    while (cmd_end) {
        size_t cmd_len = cmd_end - client->buffer;
        char* command = malloc(cmd_len + 1);
        if (command) {
            strncpy(command, client->buffer, cmd_len);
            command[cmd_len] = '\0';
            
            processCommand(server, client, command);
            free(command);
        }

        // Remove processed command from buffer
        memmove(client->buffer, cmd_end + 2, 
                client->buffer_pos - (cmd_end - client->buffer + 2));
        client->buffer_pos -= (cmd_end - client->buffer + 2);
        
        cmd_end = strstr(client->buffer, "\r\n");
    }
}

void startRedisServer(RedisServer* server) {
    if (!server) return;

    server->running = true;
    printf("Redis server started on port %d\n", DEFAULT_PORT);

    // Listen for connections
    if (listen(server->server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        return;
    }

    while (server->running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server->server_socket, &readfds);

        // Add all client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i] && server->clients[i]->connected) {
                FD_SET(server->clients[i]->socket, &readfds);
            }
        }

        // Wait for activity
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            break;
        }

        // Check for new connections
        if (FD_ISSET(server->server_socket, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server->server_socket, 
                                    (struct sockaddr*)&client_addr, 
                                    &client_len);

            if (client_socket >= 0) {
                // Find free slot for new client
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!server->clients[i]) {
                        server->clients[i] = createClient(client_socket);
                        printf("New client connected\n");
                        break;
                    }
                }
            }
        }

        // Handle client activity
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i] && 
                FD_ISSET(server->clients[i]->socket, &readfds)) {
                handleClient(server, server->clients[i]);
                
                if (!server->clients[i]->connected) {
                    freeClient(server->clients[i]);
                    server->clients[i] = NULL;
                    printf("Client disconnected\n");
                }
            }
        }
    }
}

void stopRedisServer(RedisServer* server) {
    if (server) {
        server->running = false;
    }
} 