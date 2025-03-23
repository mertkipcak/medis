#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_HASHMAP_SIZE 16  // Initial size of the hash map
#define RESIZE_UP_THRESHOLD 0.75  // Load factor threshold to increase size
#define RESIZE_DOWN_THRESHOLD 0.25  // Load factor threshold to decrease size

/**
 * Structure representing a node in the hash map.
 */
typedef struct HashNode {
    char* key;  // Key associated with the value
    unsigned char* value;  // Value stored in the node
    size_t valueSize;  // Size of the value in bytes
    struct HashNode* next;  // Pointer to the next node in case of collision
} HashNode;

/**
 * Structure representing the hash map.
 */
typedef struct HashMap {
    size_t size;  // Number of buckets in the hash map
    size_t itemCount;  // Number of stored elements
    HashNode** buckets;  // Array of pointers to hash nodes (buckets)
} HashMap;

/**
 * Creates a new hash map.
 * @return Pointer to the newly created hash map.
 */
HashMap* createHashMap();

/**
 * Hash function to generate an index for a given key.
 * @param key The key to hash.
 * @param size The size of the hash map.
 * @return The computed hash index.
 */
unsigned int hashFunction(const char* key, size_t size);

/**
 * Inserts a key-value pair into the hash map.
 * @param map The hash map.
 * @param key The key.
 * @param value The value associated with the key.
 * @param valueSize The size of the value.
 */
void hashMapInsert(HashMap* map, const char* key, unsigned char* value, size_t valueSize);

/**
 * Retrieves a value from the hash map.
 * @param map The hash map.
 * @param key The key to search for.
 * @param foundSize Pointer to store the size of the found value.
 * @return Pointer to the value if found, NULL otherwise.
 */
unsigned char* hashMapGet(HashMap* map, const char* key, size_t* foundSize);

/**
 * Removes a key-value pair from the hash map.
 * @param map The hash map.
 * @param key The key to remove.
 */
void hashMapRemove(HashMap* map, const char* key);

/**
 * Frees the memory allocated for the hash map.
 * @param map The hash map to free.
 */
void freeHashMap(HashMap* map);

/**
 * Resizes the hash map based on the scale factor.
 * @param map The hash map.
 * @param scale The scaling factor (e.g., 2.0 to double, 0.5 to halve).
 */
void resizeHashMap(HashMap* map, double scale);

/**
 * Frees a bucket and its contents.
 * @param toRemove The bucket to free.
 */
void freeBucket(HashNode* toRemove);

/**
 * Prints the contents of the hash map.
 * @param map The hash map to print.
 */
void printHashMap(HashMap* map);

#endif // HASHMAP_H
