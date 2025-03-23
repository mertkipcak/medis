#include "redis_types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// RedisObject implementation
RedisObject* createRedisObject(RedisType type, void* data) {
    RedisObject* obj = malloc(sizeof(RedisObject));
    if (!obj) return NULL;
    
    obj->type = type;
    obj->data = data;
    obj->next = NULL;
    return obj;
}

void freeRedisObject(RedisObject* obj) {
    if (!obj) return;
    
    switch (obj->type) {
        case REDIS_STRING:
            freeRedisString(obj->data);
            break;
        case REDIS_LIST:
            freeRedisList(obj->data);
            break;
        case REDIS_SET:
            freeRedisSet(obj->data);
            break;
        case REDIS_SORTED_SET:
            freeRedisSortedSet(obj->data);
            break;
        case REDIS_HASH:
            freeRedisHash(obj->data);
            break;
        case REDIS_BITMAP:
            freeRedisBitmap(obj->data);
            break;
        case REDIS_HYPERLOGLOG:
            freeRedisHyperLogLog(obj->data);
            break;
        case REDIS_GEO:
            freeRedisGeo(obj->data);
            break;
        case REDIS_STREAM:
            freeRedisStream(obj->data);
            break;
    }
    
    free(obj);
}

// String implementation
RedisString* createRedisString(const char* value) {
    RedisString* str = malloc(sizeof(RedisString));
    if (!str) return NULL;
    
    str->len = strlen(value);
    str->value = malloc(str->len + 1);
    if (!str->value) {
        free(str);
        return NULL;
    }
    
    strcpy(str->value, value);
    return str;
}

void freeRedisString(RedisString* str) {
    if (!str) return;
    free(str->value);
    free(str);
}

// List implementation
RedisList* createRedisList(void) {
    RedisList* list = malloc(sizeof(RedisList));
    if (!list) return NULL;
    
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
    return list;
}

void freeRedisList(RedisList* list) {
    if (!list) return;
    
    ListNode* current = list->head;
    while (current) {
        ListNode* next = current->next;
        free(current->value);
        free(current);
        current = next;
    }
    
    free(list);
}

void listPush(RedisList* list, const char* value, bool head) {
    ListNode* node = malloc(sizeof(ListNode));
    if (!node) return;
    
    node->value = strdup(value);
    node->prev = NULL;
    node->next = NULL;
    
    if (list->len == 0) {
        list->head = node;
        list->tail = node;
    } else if (head) {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    } else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }
    
    list->len++;
}

char* listPop(RedisList* list, bool head) {
    if (!list || list->len == 0) return NULL;
    
    ListNode* node;
    if (head) {
        node = list->head;
        list->head = node->next;
        if (list->head) list->head->prev = NULL;
    } else {
        node = list->tail;
        list->tail = node->prev;
        if (list->tail) list->tail->next = NULL;
    }
    
    char* value = node->value;
    free(node);
    list->len--;
    return value;
}

char* listIndex(RedisList* list, int64_t index) {
    if (!list || list->len == 0) return NULL;
    
    if (index < 0) index += list->len;
    if (index < 0 || index >= list->len) return NULL;
    
    ListNode* current;
    if (index < list->len / 2) {
        current = list->head;
        for (int64_t i = 0; i < index; i++) {
            current = current->next;
        }
    } else {
        current = list->tail;
        for (int64_t i = list->len - 1; i > index; i--) {
            current = current->prev;
        }
    }
    
    return strdup(current->value);
}

// Set implementation
RedisSet* createRedisSet(void) {
    RedisSet* set = malloc(sizeof(RedisSet));
    if (!set) return NULL;
    
    set->size = 0;
    set->capacity = 16;
    set->elements = malloc(set->capacity * sizeof(char*));
    if (!set->elements) {
        free(set);
        return NULL;
    }
    
    return set;
}

void freeRedisSet(RedisSet* set) {
    if (!set) return;
    
    for (size_t i = 0; i < set->size; i++) {
        free(set->elements[i]);
    }
    free(set->elements);
    free(set);
}

bool setAdd(RedisSet* set, const char* member) {
    if (!set || !member) return false;
    
    // Check if member already exists
    for (size_t i = 0; i < set->size; i++) {
        if (strcmp(set->elements[i], member) == 0) {
            return false;
        }
    }
    
    // Resize if needed
    if (set->size >= set->capacity) {
        size_t new_capacity = set->capacity * 2;
        char** new_elements = realloc(set->elements, new_capacity * sizeof(char*));
        if (!new_elements) return false;
        
        set->elements = new_elements;
        set->capacity = new_capacity;
    }
    
    set->elements[set->size] = strdup(member);
    set->size++;
    return true;
}

bool setRemove(RedisSet* set, const char* member) {
    if (!set || !member) return false;
    
    for (size_t i = 0; i < set->size; i++) {
        if (strcmp(set->elements[i], member) == 0) {
            free(set->elements[i]);
            memmove(&set->elements[i], &set->elements[i + 1], 
                   (set->size - i - 1) * sizeof(char*));
            set->size--;
            return true;
        }
    }
    
    return false;
}

bool setIsMember(RedisSet* set, const char* member) {
    if (!set || !member) return false;
    
    for (size_t i = 0; i < set->size; i++) {
        if (strcmp(set->elements[i], member) == 0) {
            return true;
        }
    }
    
    return false;
}

// Sorted Set implementation (using skiplist)
static size_t randomLevel(void) {
    size_t level = 1;
    while (rand() < RAND_MAX / 2 && level < 32) {
        level++;
    }
    return level;
}

RedisSortedSet* createRedisSortedSet(void) {
    RedisSortedSet* zset = malloc(sizeof(RedisSortedSet));
    if (!zset) return NULL;
    
    zset->header = malloc(sizeof(SkipListNode));
    if (!zset->header) {
        free(zset);
        return NULL;
    }
    
    zset->header->member = NULL;
    zset->header->score = 0;
    zset->header->level = 32;
    zset->header->forward = calloc(32, sizeof(SkipListNode*));
    if (!zset->header->forward) {
        free(zset->header);
        free(zset);
        return NULL;
    }
    
    zset->length = 0;
    zset->level = 1;
    return zset;
}

void freeRedisSortedSet(RedisSortedSet* zset) {
    if (!zset) return;
    
    SkipListNode* current = zset->header;
    while (current) {
        SkipListNode* next = current->forward[0];
        free(current->member);
        free(current->forward);
        free(current);
        current = next;
    }
    
    free(zset);
}

bool zsetAdd(RedisSortedSet* zset, const char* member, double score) {
    if (!zset || !member) return false;
    
    SkipListNode* update[32] = {0};
    SkipListNode* current = zset->header;
    
    // Find insertion point
    for (int i = zset->level - 1; i >= 0; i--) {
        while (current->forward[i] && 
               (current->forward[i]->score < score || 
                (current->forward[i]->score == score && 
                 strcmp(current->forward[i]->member, member) < 0))) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    // Check if member already exists
    if (current->forward[0] && 
        strcmp(current->forward[0]->member, member) == 0) {
        current->forward[0]->score = score;
        return true;
    }
    
    // Create new node
    size_t level = randomLevel();
    if (level > zset->level) {
        for (size_t i = zset->level; i < level; i++) {
            update[i] = zset->header;
        }
        zset->level = level;
    }
    
    SkipListNode* node = malloc(sizeof(SkipListNode));
    if (!node) return false;
    
    node->member = strdup(member);
    node->score = score;
    node->level = level;
    node->forward = calloc(level, sizeof(SkipListNode*));
    if (!node->forward) {
        free(node->member);
        free(node);
        return false;
    }
    
    // Update forward pointers
    for (size_t i = 0; i < level; i++) {
        node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = node;
    }
    
    zset->length++;
    return true;
}

// Hash implementation
RedisHash* createRedisHash(void) {
    RedisHash* hash = malloc(sizeof(RedisHash));
    if (!hash) return NULL;
    
    hash->size = 0;
    hash->capacity = 16;
    hash->fields = malloc(hash->capacity * sizeof(char*));
    hash->values = malloc(hash->capacity * sizeof(char*));
    
    if (!hash->fields || !hash->values) {
        free(hash->fields);
        free(hash->values);
        free(hash);
        return NULL;
    }
    
    return hash;
}

void freeRedisHash(RedisHash* hash) {
    if (!hash) return;
    
    for (size_t i = 0; i < hash->size; i++) {
        free(hash->fields[i]);
        free(hash->values[i]);
    }
    free(hash->fields);
    free(hash->values);
    free(hash);
}

bool hashSet(RedisHash* hash, const char* field, const char* value) {
    if (!hash || !field || !value) return false;
    
    // Check if field exists
    for (size_t i = 0; i < hash->size; i++) {
        if (strcmp(hash->fields[i], field) == 0) {
            free(hash->values[i]);
            hash->values[i] = strdup(value);
            return true;
        }
    }
    
    // Resize if needed
    if (hash->size >= hash->capacity) {
        size_t new_capacity = hash->capacity * 2;
        char** new_fields = realloc(hash->fields, new_capacity * sizeof(char*));
        char** new_values = realloc(hash->values, new_capacity * sizeof(char*));
        
        if (!new_fields || !new_values) {
            free(new_fields);
            free(new_values);
            return false;
        }
        
        hash->fields = new_fields;
        hash->values = new_values;
        hash->capacity = new_capacity;
    }
    
    hash->fields[hash->size] = strdup(field);
    hash->values[hash->size] = strdup(value);
    hash->size++;
    return true;
}

char* hashGet(RedisHash* hash, const char* field) {
    if (!hash || !field) return NULL;
    
    for (size_t i = 0; i < hash->size; i++) {
        if (strcmp(hash->fields[i], field) == 0) {
            return strdup(hash->values[i]);
        }
    }
    
    return NULL;
}

bool hashDelete(RedisHash* hash, const char* field) {
    if (!hash || !field) return false;
    
    for (size_t i = 0; i < hash->size; i++) {
        if (strcmp(hash->fields[i], field) == 0) {
            free(hash->fields[i]);
            free(hash->values[i]);
            memmove(&hash->fields[i], &hash->fields[i + 1], 
                   (hash->size - i - 1) * sizeof(char*));
            memmove(&hash->values[i], &hash->values[i + 1], 
                   (hash->size - i - 1) * sizeof(char*));
            hash->size--;
            return true;
        }
    }
    
    return false;
}

// Bitmap implementation
RedisBitmap* createRedisBitmap(void) {
    RedisBitmap* bitmap = malloc(sizeof(RedisBitmap));
    if (!bitmap) return NULL;
    
    bitmap->size = 1024;  // Initial size in bits
    bitmap->bits = calloc((bitmap->size + 7) / 8, sizeof(uint8_t));
    if (!bitmap->bits) {
        free(bitmap);
        return NULL;
    }
    
    return bitmap;
}

void freeRedisBitmap(RedisBitmap* bitmap) {
    if (!bitmap) return;
    free(bitmap->bits);
    free(bitmap);
}

bool bitmapSet(RedisBitmap* bitmap, size_t offset, bool value) {
    if (!bitmap) return false;
    
    // Resize if needed
    if (offset >= bitmap->size) {
        size_t new_size = offset + 1;
        size_t new_bytes = (new_size + 7) / 8;
        uint8_t* new_bits = realloc(bitmap->bits, new_bytes);
        if (!new_bits) return false;
        
        // Clear new bits
        memset(new_bits + (bitmap->size + 7) / 8, 0, 
               new_bytes - (bitmap->size + 7) / 8);
        
        bitmap->bits = new_bits;
        bitmap->size = new_size;
    }
    
    size_t byte_index = offset / 8;
    size_t bit_index = offset % 8;
    
    if (value) {
        bitmap->bits[byte_index] |= (1 << bit_index);
    } else {
        bitmap->bits[byte_index] &= ~(1 << bit_index);
    }
    
    return true;
}

bool bitmapGet(RedisBitmap* bitmap, size_t offset) {
    if (!bitmap || offset >= bitmap->size) return false;
    
    size_t byte_index = offset / 8;
    size_t bit_index = offset % 8;
    
    return (bitmap->bits[byte_index] & (1 << bit_index)) != 0;
}

size_t bitmapCount(RedisBitmap* bitmap) {
    if (!bitmap) return 0;
    
    size_t count = 0;
    for (size_t i = 0; i < bitmap->size; i++) {
        if (bitmapGet(bitmap, i)) count++;
    }
    return count;
}

// HyperLogLog implementation
RedisHyperLogLog* createRedisHyperLogLog(void) {
    RedisHyperLogLog* hll = malloc(sizeof(RedisHyperLogLog));
    if (!hll) return NULL;
    
    hll->size = 16384;  // 2^14 registers
    hll->registers = calloc(hll->size, sizeof(uint8_t));
    if (!hll->registers) {
        free(hll);
        return NULL;
    }
    
    return hll;
}

void freeRedisHyperLogLog(RedisHyperLogLog* hll) {
    if (!hll) return;
    free(hll->registers);
    free(hll);
}

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

void hllAdd(RedisHyperLogLog* hll, const char* element) {
    if (!hll || !element) return;
    
    uint32_t hash = murmurhash2(element, strlen(element));
    uint32_t index = hash & (hll->size - 1);
    uint32_t value = hash >> 14;
    
    uint8_t count = 1;
    while (value && count < 32) {
        value >>= 1;
        count++;
    }
    
    if (count > hll->registers[index]) {
        hll->registers[index] = count;
    }
}

uint64_t hllCount(RedisHyperLogLog* hll) {
    if (!hll) return 0;
    
    double sum = 0;
    for (size_t i = 0; i < hll->size; i++) {
        sum += 1.0 / (1 << hll->registers[i]);
    }
    
    double alpha = 0.7213 / (1 + 1.079 / hll->size);
    double estimate = alpha * hll->size * hll->size / sum;
    
    if (estimate < 2.5 * hll->size) {
        size_t zeros = 0;
        for (size_t i = 0; i < hll->size; i++) {
            if (hll->registers[i] == 0) zeros++;
        }
        if (zeros != 0) {
            estimate = hll->size * log((double)hll->size / zeros);
        }
    }
    
    return (uint64_t)estimate;
}

// Geo implementation
RedisGeo* createRedisGeo(void) {
    RedisGeo* geo = malloc(sizeof(RedisGeo));
    if (!geo) return NULL;
    
    geo->size = 0;
    geo->capacity = 16;
    geo->points = malloc(geo->capacity * sizeof(GeoPoint));
    if (!geo->points) {
        free(geo);
        return NULL;
    }
    
    return geo;
}

void freeRedisGeo(RedisGeo* geo) {
    if (!geo) return;
    
    for (size_t i = 0; i < geo->size; i++) {
        free(geo->points[i].member);
    }
    free(geo->points);
    free(geo);
}

static double geohash(double longitude, double latitude) {
    // Simple geohash implementation
    return (longitude + 180) * 360 + (latitude + 90);
}

bool geoAdd(RedisGeo* geo, const char* member, double longitude, double latitude) {
    if (!geo || !member) return false;
    
    // Check if member exists
    for (size_t i = 0; i < geo->size; i++) {
        if (strcmp(geo->points[i].member, member) == 0) {
            geo->points[i].longitude = longitude;
            geo->points[i].latitude = latitude;
            geo->points[i].score = geohash(longitude, latitude);
            return true;
        }
    }
    
    // Resize if needed
    if (geo->size >= geo->capacity) {
        size_t new_capacity = geo->capacity * 2;
        GeoPoint* new_points = realloc(geo->points, new_capacity * sizeof(GeoPoint));
        if (!new_points) return false;
        
        geo->points = new_points;
        geo->capacity = new_capacity;
    }
    
    geo->points[geo->size].member = strdup(member);
    geo->points[geo->size].longitude = longitude;
    geo->points[geo->size].latitude = latitude;
    geo->points[geo->size].score = geohash(longitude, latitude);
    geo->size++;
    return true;
}

GeoPoint* geoGet(RedisGeo* geo, const char* member) {
    if (!geo || !member) return NULL;
    
    for (size_t i = 0; i < geo->size; i++) {
        if (strcmp(geo->points[i].member, member) == 0) {
            return &geo->points[i];
        }
    }
    
    return NULL;
}

static double haversine(double lat1, double lon1, double lat2, double lon2) {
    double R = 6371;  // Earth's radius in km
    double dLat = (lat2 - lat1) * M_PI / 180;
    double dLon = (lon2 - lon1) * M_PI / 180;
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1 * M_PI / 180) * cos(lat2 * M_PI / 180) *
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
}

double* geoDistance(RedisGeo* geo, const char* member1, const char* member2) {
    if (!geo || !member1 || !member2) return NULL;
    
    GeoPoint* p1 = geoGet(geo, member1);
    GeoPoint* p2 = geoGet(geo, member2);
    
    if (!p1 || !p2) return NULL;
    
    double* distance = malloc(sizeof(double));
    if (!distance) return NULL;
    
    *distance = haversine(p1->latitude, p1->longitude, 
                         p2->latitude, p2->longitude);
    return distance;
}

// Stream implementation
RedisStream* createRedisStream(void) {
    RedisStream* stream = malloc(sizeof(RedisStream));
    if (!stream) return NULL;
    
    stream->first = NULL;
    stream->last = NULL;
    stream->length = 0;
    return stream;
}

void freeRedisStream(RedisStream* stream) {
    if (!stream) return;
    
    StreamEntry* current = stream->first;
    while (current) {
        StreamEntry* next = current->next;
        free(current->id);
        for (size_t i = 0; i < current->num_fields; i++) {
            free(current->fields[i]);
            free(current->values[i]);
        }
        free(current->fields);
        free(current->values);
        free(current);
        current = next;
    }
    
    free(stream);
}

StreamEntry* streamAdd(RedisStream* stream, const char* id, 
                      char** fields, char** values, size_t num_fields) {
    if (!stream || !id || !fields || !values || num_fields == 0) return NULL;
    
    StreamEntry* entry = malloc(sizeof(StreamEntry));
    if (!entry) return NULL;
    
    entry->id = strdup(id);
    entry->num_fields = num_fields;
    entry->fields = malloc(num_fields * sizeof(char*));
    entry->values = malloc(num_fields * sizeof(char*));
    
    if (!entry->fields || !entry->values) {
        free(entry->id);
        free(entry->fields);
        free(entry->values);
        free(entry);
        return NULL;
    }
    
    for (size_t i = 0; i < num_fields; i++) {
        entry->fields[i] = strdup(fields[i]);
        entry->values[i] = strdup(values[i]);
    }
    
    entry->next = NULL;
    
    if (stream->length == 0) {
        stream->first = entry;
        stream->last = entry;
    } else {
        stream->last->next = entry;
        stream->last = entry;
    }
    
    stream->length++;
    return entry;
}

StreamEntry* streamGet(RedisStream* stream, const char* id) {
    if (!stream || !id) return NULL;
    
    StreamEntry* current = stream->first;
    while (current) {
        if (strcmp(current->id, id) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

void streamDelete(RedisStream* stream, const char* id) {
    if (!stream || !id) return;
    
    StreamEntry* current = stream->first;
    StreamEntry* prev = NULL;
    
    while (current) {
        if (strcmp(current->id, id) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                stream->first = current->next;
            }
            
            if (current == stream->last) {
                stream->last = prev;
            }
            
            free(current->id);
            for (size_t i = 0; i < current->num_fields; i++) {
                free(current->fields[i]);
                free(current->values[i]);
            }
            free(current->fields);
            free(current->values);
            free(current);
            
            stream->length--;
            return;
        }
        
        prev = current;
        current = current->next;
    }
} 