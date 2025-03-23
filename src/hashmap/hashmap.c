//
// Created by Mert Kipcak on 2025-02-22.
//

#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define MAX_LOAD_FACTOR 0.75f

// MurmurHash2 implementation
static uint32_t murmurhash2(const char* key, size_t len) {
    const uint32_t seed = 0x1BADB002;
    const uint32_t m = 0x5BD1E995;
    const int r = 24;
    uint32_t h = seed ^ len;
    const unsigned char* data = (const unsigned char*)key;
    
    while (len >= 4) {
        uint32_t k = *(uint32_t*)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }
    
    switch (len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
    }
    
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    
    return h;
}

// Hash function
static size_t hash(const char* key, size_t capacity) {
    return murmurhash2(key, strlen(key)) % capacity;
}

// Resize the hashmap
static void resize(Hashmap* map) {
    size_t old_capacity = map->capacity;
    RedisObject** old_buckets = map->buckets;
    
    map->capacity *= 2;
    map->buckets = calloc(map->capacity, sizeof(RedisObject*));
    if (!map->buckets) {
        map->capacity = old_capacity;
        map->buckets = old_buckets;
        return;
    }
    
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_buckets[i]) {
            RedisObject* obj = old_buckets[i];
            size_t index = hash(obj->key, map->capacity);
            map->buckets[index] = obj;
        }
    }
    
    free(old_buckets);
}

Hashmap* hashmap_create(size_t initial_capacity) {
    Hashmap* map = malloc(sizeof(Hashmap));
    if (!map) return NULL;
    
    map->capacity = initial_capacity ? initial_capacity : INITIAL_CAPACITY;
    map->size = 0;
    map->load_factor = 0.0f;
    map->buckets = calloc(map->capacity, sizeof(RedisObject*));
    
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    
    return map;
}

void hashmap_destroy(Hashmap* map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->buckets[i]) {
            freeRedisObject(map->buckets[i]);
        }
    }
    
    free(map->buckets);
    free(map);
}

bool hashmap_put(Hashmap* map, const char* key, RedisObject* value) {
    if (!map || !key || !value) return false;
    
    // Check load factor and resize if needed
    if (map->load_factor >= MAX_LOAD_FACTOR) {
        resize(map);
    }
    
    size_t index = hash(key, map->capacity);
    
    // Handle collision by chaining
    if (map->buckets[index]) {
        RedisObject* current = map->buckets[index];
        while (current->next) {
            if (strcmp(current->key, key) == 0) {
                // Update existing key
                freeRedisObject(current);
                map->buckets[index] = value;
                value->next = current->next;
                return true;
            }
            current = current->next;
        }
        
        if (strcmp(current->key, key) == 0) {
            // Update existing key
            freeRedisObject(current);
            map->buckets[index] = value;
            value->next = current->next;
            return true;
        }
        
        // Add to chain
        value->next = map->buckets[index];
        map->buckets[index] = value;
    } else {
        map->buckets[index] = value;
        value->next = NULL;
    }
    
    map->size++;
    map->load_factor = (float)map->size / map->capacity;
    return true;
}

RedisObject* hashmap_get(Hashmap* map, const char* key) {
    if (!map || !key) return NULL;
    
    size_t index = hash(key, map->capacity);
    RedisObject* current = map->buckets[index];
    
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

bool hashmap_remove(Hashmap* map, const char* key) {
    if (!map || !key) return false;
    
    size_t index = hash(key, map->capacity);
    RedisObject* current = map->buckets[index];
    RedisObject* prev = NULL;
    
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->buckets[index] = current->next;
            }
            
            freeRedisObject(current);
            map->size--;
            map->load_factor = (float)map->size / map->capacity;
            return true;
        }
        
        prev = current;
        current = current->next;
    }
    
    return false;
}

bool hashmap_contains(Hashmap* map, const char* key) {
    return hashmap_get(map, key) != NULL;
}

size_t hashmap_size(Hashmap* map) {
    return map ? map->size : 0;
}

void hashmap_clear(Hashmap* map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->buckets[i]) {
            freeRedisObject(map->buckets[i]);
            map->buckets[i] = NULL;
        }
    }
    
    map->size = 0;
    map->load_factor = 0.0f;
}
