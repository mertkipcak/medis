# Medis - A Redis-like Key-Value Store

Medis is a lightweight, in-memory key-value store implementation inspired by Redis. It provides a simple and efficient way to store and retrieve data using a hash-based storage system.

## Features

- In-memory key-value storage
- Dynamic hash map resizing
- Collision handling
- RESP (Redis Serialization Protocol) support
- Basic Redis commands (SET, GET, DEL)
- Multi-client support
- Unit tests with CUnit

## Building

### Prerequisites

- GCC compiler
- Make
- CUnit testing framework

On macOS, install CUnit using:
```bash
brew install cunit
```

### Compilation

Build the main application:
```bash
make
```

Run the test suite:
```bash
make test
```

Clean build artifacts:
```bash
make clean
```

## Usage

Start the server:
```bash
./medis
```

Connect using Redis CLI:
```bash
redis-cli -p 6379
```

## Project Structure

```
medis/
├── src/
│   ├── hashmap/         # Hash map implementation
│   │   ├── hashmap.c
│   │   └── hashmap.h
│   ├── server/          # Redis server implementation
│   │   ├── redis_server.c
│   │   └── redis_server.h
│   └── main.c          # Main application entry point
├── tests/              # Test files
│   ├── test_hashmap.c
│   ├── test_hashmap.h
│   ├── test_redis_server.c
│   ├── test_redis_server.h
│   └── test_main.c
└── Makefile
```

## Documentation

### HashMap Implementation

The hash map implementation provides a dynamic, resizable key-value store with the following features:

- Automatic resizing based on load factor
- Collision handling using chaining
- Memory-efficient storage
- Thread-safe operations

Key functions:
- `createHashMap()`: Creates a new hash map instance
- `hashMapInsert()`: Inserts a key-value pair
- `hashMapGet()`: Retrieves a value by key
- `hashMapRemove()`: Removes a key-value pair
- `freeHashMap()`: Frees allocated memory

### Redis Server Implementation

The Redis server implementation provides a network interface for the hash map storage with the following features:

- TCP server implementation
- RESP protocol support
- Multi-client handling
- Basic Redis commands

Key functions:
- `createRedisServer()`: Creates a new Redis server instance
- `startRedisServer()`: Starts the server
- `stopRedisServer()`: Stops the server
- `processCommand()`: Processes Redis commands
- `handleClient()`: Handles client connections

### Supported Commands

- `SET key value`: Set a key-value pair
- `GET key`: Get the value for a key
- `DEL key`: Delete a key

## Testing

The project includes a comprehensive test suite using CUnit. Tests cover:

- HashMap operations
- Collision handling
- Dynamic resizing
- Redis server functionality
- Command processing
- Client handling

Run tests:
```bash
make test
```

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a new Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.