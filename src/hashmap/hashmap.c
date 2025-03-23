//
// Created by Mert Kipcak on 2025-02-22.
//

#include "hashmap.h"

HashMap* createHashMap() {
    HashMap* map = (HashMap*) malloc(sizeof(HashMap));
    if (map == NULL) return NULL;

    map->size = INITIAL_HASHMAP_SIZE;
    map->buckets = calloc(map->size, sizeof(HashNode*));
    if(!map->buckets) {
        free(map);
        return NULL;
    }

    return map;
}

unsigned int hashFunction(const char* key, size_t size) {
    unsigned int h = 0x9747b28c;
    for (; *key; key++) {
        h ^= *key;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h % size; 
}

void hashMapInsert(HashMap* map, const char* key, unsigned char* value, size_t valueSize) {
    unsigned int index = hashFunction(key, map->size);

    HashNode* newNode = malloc(sizeof(HashNode));
    if (!newNode) return;

    newNode->value = malloc(valueSize * sizeof(unsigned char));
    if (!newNode->value) {
        free(newNode);
        return;
    }
    newNode->key = strdup(key);
    if (!newNode->key) {
        free(newNode->value);
        free(newNode);
        return;
    }

    memcpy(newNode->value, value, valueSize);

    newNode->valueSize = valueSize;

    newNode->next = map->buckets[index];
    map->buckets[index] = newNode;
    
    map->itemCount++;
    if ((double) map->itemCount / map->size > RESIZE_UP_THRESHOLD) {
        resizeHashMap(map, 2.0);
    } else if ((double) map->itemCount / map->size < RESIZE_DOWN_THRESHOLD && map->size > INITIAL_HASHMAP_SIZE) {
        resizeHashMap(map, 0.5);
    }
}

void resizeHashMap(HashMap* map, double scale) {
    size_t newSize = map->size * scale;
    HashNode** newBuckets = calloc(newSize, sizeof(HashNode*));

    for (size_t i = 0; i < map->size; i++) {
        HashNode* node = map->buckets[i];
        while (node) {
            unsigned int newIndex = hashFunction(node->key, newSize);

            HashNode* next = node->next;
            node->next = newBuckets[newIndex];
            newBuckets[newIndex] = node;

            node = next;
        }
    }

    free(map->buckets);
    map->buckets = newBuckets;
    map->size = newSize;
}


unsigned char* hashMapGet(HashMap* map, const char* key, size_t* foundSize) {
    unsigned int index = hashFunction(key, map->size);

    HashNode* fetchedNode = map->buckets[index];

    while(fetchedNode && strcmp(key, fetchedNode->key) != 0) {
        fetchedNode = fetchedNode->next;
    }

    if (!fetchedNode) return NULL;

    *foundSize = fetchedNode->valueSize;

    return fetchedNode->value;
}

void hashMapRemove(HashMap* map, const char* key) {
    unsigned int index = hashFunction(key, map->size);
    HashNode* node = map->buckets[index];
    HashNode* prev = NULL;

    while (node && strcmp(key, node->key) != 0) {
        prev = node;
        node = node->next;
    }

    if (!node) return;

    if (prev) {
        prev->next = node->next;
    } else {
        map->buckets[index] = node->next;
    }

    free(node->key);
    free(node->value);
    free(node);

    map->itemCount--;
}

void freeHashMap(HashMap* map) {
    for(int i = 0; i < (int) map->size; i++) {
        freeBucket(map->buckets[i]);
    }
    free(map->buckets);
    free(map);
}

void freeBucket(HashNode* toRemove) {
    while (toRemove) {
        HashNode* next = toRemove->next;

        free(toRemove->key);
        free(toRemove->value);
        free(toRemove);

        toRemove = next;
    }
}

void printHashMap(HashMap* map) {
    for (int i = 0; i < (int) map->size; i++) {
        HashNode* node = map->buckets[i];
        if (node) {
            printf("Bucket %d:\n", i);
        }
        while (node) {
            printf("  Key: %s, Value: ", node->key);
            for (size_t j = 0; j < node->valueSize; j++) {
            printf("%02x ", node->value[j]);
            }
            printf("\n");
            node = node->next;
        }
    }
}
