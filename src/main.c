//
// Created by Mert Kipcak on 2025-02-22.
//
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "server/redis_server.h"

RedisServer* server = NULL;

void signal_handler(int signum) {
    if (server) {
        printf("\nShutting down Redis server...\n");
        stopRedisServer(server);
    }
}

int main() {
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create and start server
    server = createRedisServer(DEFAULT_PORT);
    if (!server) {
        fprintf(stderr, "Failed to create Redis server\n");
        return 1;
    }

    startRedisServer(server);

    // Cleanup
    freeRedisServer(server);
    return 0;
}