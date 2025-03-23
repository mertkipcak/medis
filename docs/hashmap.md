# HashMap Implementation Documentation

## Overview

The HashMap implementation provides a dynamic, resizable key-value store that serves as the core storage engine for the Medis project. It uses a hash-based approach with chaining for collision resolution.

## Data Structures

### HashNode
```c
typedef struct HashNode {
    char* key;              // Key string
    unsigned char* value;   // Value data
    size_t valueSize;       // Size of value in bytes
    struct HashNode* next;  // Pointer to next node (for chaining)
} HashNode;
```

### HashMap
```c
typedef struct HashMap {
    size_t size;           // Number of buckets
    size_t itemCount;      // Number of stored items
    HashNode** buckets;    // Array of bucket pointers
} HashMap;
```

## Constants

- `INITIAL_HASHMAP_SIZE`: Initial number of buckets (16)
- `RESIZE_UP_THRESHOLD`: Load factor threshold for increasing size (0.75)
- `RESIZE_DOWN_THRESHOLD`: Load factor threshold for decreasing size (0.25)

## Functions

### createHashMap
```c
HashMap* createHashMap(void)
```
Creates a new hash map instance.
- **Returns**: Pointer to the newly created hash map
- **Memory**: Allocates memory for the hash map structure and bucket array

### hashFunction
```c
unsigned int hashFunction(const char* key, size_t size)
```
Computes the hash value for a given key.
- **Parameters**:
  - `key`: The key string to hash
  - `size`: The size of the hash map
- **Returns**: Hash value between 0 and size-1

### hashMapInsert
```c
void hashMapInsert(HashMap* map, const char* key, unsigned char* value, size_t valueSize)
```
Inserts a key-value pair into the hash map.
- **Parameters**:
  - `map`: The hash map to insert into
  - `key`: The key string
  - `value`: The value data
  - `valueSize`: Size of the value in bytes
- **Behavior**:
  - Creates a new node if key doesn't exist
  - Updates existing node if key exists
  - Triggers resize if load factor exceeds threshold

### hashMapGet
```c
unsigned char* hashMapGet(HashMap* map, const char* key, size_t* foundSize)
```
Retrieves a value by key.
- **Parameters**:
  - `map`: The hash map to search in
  - `key`: The key to search for
  - `foundSize`: Pointer to store the size of the found value
- **Returns**: Pointer to the value if found, NULL otherwise

### hashMapRemove
```c
void hashMapRemove(HashMap* map, const char* key)
```
Removes a key-value pair from the hash map.
- **Parameters**:
  - `map`: The hash map to remove from
  - `key`: The key to remove
- **Behavior**:
  - Removes the node if found
  - Triggers resize if load factor falls below threshold

### resizeHashMap
```c
void resizeHashMap(HashMap* map, double scale)
```
Resizes the hash map by a given scale factor.
- **Parameters**:
  - `map`: The hash map to resize
  - `scale`: The scaling factor (e.g., 2.0 to double, 0.5 to halve)
- **Behavior**:
  - Creates new bucket array
  - Rehashes all existing items
  - Frees old bucket array

### freeHashMap
```c
void freeHashMap(HashMap* map)
```
Frees all memory allocated for the hash map.
- **Parameters**:
  - `map`: The hash map to free
- **Behavior**:
  - Frees all nodes
  - Frees bucket array
  - Frees hash map structure

## Memory Management

The implementation handles memory management carefully:
- All memory allocations are checked for failure
- Memory is properly freed in all cases
- No memory leaks in normal operation
- Proper cleanup in error cases

## Thread Safety

The current implementation is not thread-safe. For multi-threaded use:
- External synchronization is required
- Consider adding mutex locks for critical sections
- Implement read-write locks for better concurrency

## Performance Characteristics

- Average case O(1) for insert, get, and remove operations
- Worst case O(n) when many collisions occur
- Automatic resizing maintains good performance
- Memory overhead is proportional to number of items

## Usage Example

```c
// Create a new hash map
HashMap* map = createHashMap();

// Insert a key-value pair
const char* key = "mykey";
unsigned char value[] = "myvalue";
hashMapInsert(map, key, value, sizeof(value));

// Retrieve a value
size_t foundSize;
unsigned char* retrieved = hashMapGet(map, key, &foundSize);

// Remove a key-value pair
hashMapRemove(map, key);

// Clean up
freeHashMap(map);
``` 