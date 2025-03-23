#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>  // for size_t
#include "../types/redis_types.h"

// Hash map structure
typedef struct {
    RedisObject** buckets;
    size_t size;
    size_t capacity;
    float load_factor;
} Hashmap;

// Function declarations
Hashmap* hashmap_create(size_t initial_capacity);
void hashmap_destroy(Hashmap* map);
bool hashmap_put(Hashmap* map, const char* key, RedisObject* value);
RedisObject* hashmap_get(Hashmap* map, const char* key);
bool hashmap_remove(Hashmap* map, const char* key);
bool hashmap_contains(Hashmap* map, const char* key);
size_t hashmap_size(Hashmap* map);
void hashmap_clear(Hashmap* map);

#endif // HASHMAP_H
