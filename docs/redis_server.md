# Redis Server Implementation Documentation

## Overview

The Redis server implementation provides a network interface for the hash map storage, implementing a subset of the Redis protocol (RESP). It handles client connections, command processing, and response formatting.

## Data Structures

### Client
```c
typedef struct {
    int socket;              // Client socket descriptor
    char buffer[BUFFER_SIZE]; // Command buffer
    size_t buffer_pos;       // Current position in buffer
    bool connected;          // Connection status
} Client;
```

### RedisServer
```c
typedef struct {
    int server_socket;       // Server socket descriptor
    Client* clients[MAX_CLIENTS]; // Array of client connections
    HashMap* db;            // Database instance
    bool running;           // Server running status
} RedisServer;
```

## Constants

- `MAX_CLIENTS`: Maximum number of concurrent clients (100)
- `BUFFER_SIZE`: Size of client command buffer (1024)
- `DEFAULT_PORT`: Default server port (6379)

## Functions

### createRedisServer
```c
RedisServer* createRedisServer(uint16_t port)
```
Creates a new Redis server instance.
- **Parameters**:
  - `port`: Port number to listen on
- **Returns**: Pointer to the newly created server
- **Behavior**:
  - Initializes server structure
  - Creates database instance
  - Sets up server socket
  - Binds to specified port

### startRedisServer
```c
void startRedisServer(RedisServer* server)
```
Starts the Redis server.
- **Parameters**:
  - `server`: The server instance to start
- **Behavior**:
  - Begins listening for connections
  - Handles client connections
  - Processes client commands
  - Manages client disconnections

### stopRedisServer
```c
void stopRedisServer(RedisServer* server)
```
Stops the Redis server.
- **Parameters**:
  - `server`: The server instance to stop
- **Behavior**:
  - Sets running flag to false
  - Triggers server shutdown
  - Allows clean exit

### processCommand
```c
void processCommand(RedisServer* server, Client* client, const char* command)
```
Processes a Redis command.
- **Parameters**:
  - `server`: The server instance
  - `client`: The client sending the command
  - `command`: The command string to process
- **Behavior**:
  - Parses command
  - Executes appropriate operation
  - Sends response to client

### handleClient
```c
void handleClient(RedisServer* server, Client* client)
```
Handles client communication.
- **Parameters**:
  - `server`: The server instance
  - `client`: The client to handle
- **Behavior**:
  - Reads client data
  - Processes complete commands
  - Manages client buffer
  - Handles client disconnection

## Supported Commands

### SET
```c
SET key value
```
Sets a key-value pair in the database.
- **Response**: "OK" on success

### GET
```c
GET key
```
Retrieves a value by key.
- **Response**: Value if found, "(nil)" if not found

### DEL
```c
DEL key
```
Deletes a key-value pair.
- **Response**: "1" if deleted, "0" if not found

## RESP Protocol Implementation

The server implements a subset of the Redis Serialization Protocol (RESP):

- Simple Strings: `+OK\r\n`
- Errors: `-ERR message\r\n`
- Integers: `:1\r\n`
- Bulk Strings: `$6\r\nfoobar\r\n`
- Arrays: `*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n`

## Network Handling

The server uses:
- Non-blocking socket operations
- Select-based multiplexing
- Command buffering
- Connection pooling

## Error Handling

The implementation includes:
- Socket error handling
- Memory allocation checks
- Client disconnection handling
- Command parsing errors
- Protocol errors

## Usage Example

```c
// Create and start server
RedisServer* server = createRedisServer(6379);
if (server) {
    startRedisServer(server);
    
    // Server is now running and handling clients
    
    // Stop server when done
    stopRedisServer(server);
    freeRedisServer(server);
}
```

## Testing

The server implementation includes unit tests for:
- Server creation and initialization
- Command processing
- Client handling
- Protocol formatting
- Error cases

## Limitations

Current limitations include:
- No persistence
- Limited command set
- No authentication
- No pub/sub
- No transactions
- No Lua scripting

## Future Improvements

Potential improvements:
- Add more Redis commands
- Implement persistence
- Add authentication
- Add pub/sub support
- Add transaction support
- Add Lua scripting
- Improve error handling
- Add monitoring
- Add replication 