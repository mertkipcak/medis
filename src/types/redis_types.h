#ifndef REDIS_TYPES_H
#define REDIS_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Redis data type enumeration
typedef enum {
    REDIS_STRING,
    REDIS_LIST,
    REDIS_SET,
    REDIS_SORTED_SET,
    REDIS_HASH,
    REDIS_BITMAP,
    REDIS_HYPERLOGLOG,
    REDIS_GEO,
    REDIS_STREAM
} RedisType;

// Base structure for all Redis objects
typedef struct RedisObject {
    RedisType type;
    void* data;
    struct RedisObject* next;  // For chaining in hash map
} RedisObject;

// String type
typedef struct {
    char* value;
    size_t len;
} RedisString;

// List type (doubly linked list)
typedef struct ListNode {
    char* value;
    struct ListNode* prev;
    struct ListNode* next;
} ListNode;

typedef struct {
    ListNode* head;
    ListNode* tail;
    size_t len;
} RedisList;

// Set type (hash set)
typedef struct {
    char** elements;
    size_t size;
    size_t capacity;
} RedisSet;

// Sorted Set type (skiplist)
typedef struct SkipListNode {
    char* member;
    double score;
    struct SkipListNode** forward;
    size_t level;
} SkipListNode;

typedef struct {
    SkipListNode* header;
    size_t length;
    size_t level;
} RedisSortedSet;

// Hash type
typedef struct {
    char** fields;
    char** values;
    size_t size;
    size_t capacity;
} RedisHash;

// Bitmap type
typedef struct {
    uint8_t* bits;
    size_t size;
} RedisBitmap;

// HyperLogLog type
typedef struct {
    uint8_t* registers;
    size_t size;
} RedisHyperLogLog;

// Geo type (sorted set with geohash)
typedef struct {
    char* member;
    double longitude;
    double latitude;
    double score;  // Geohash score
} GeoPoint;

typedef struct {
    GeoPoint* points;
    size_t size;
    size_t capacity;
} RedisGeo;

// Stream type
typedef struct StreamEntry {
    char* id;
    char** fields;
    char** values;
    size_t num_fields;
    struct StreamEntry* next;
} StreamEntry;

typedef struct {
    StreamEntry* first;
    StreamEntry* last;
    size_t length;
} RedisStream;

// Function declarations
RedisObject* createRedisObject(RedisType type, void* data);
void freeRedisObject(RedisObject* obj);

// String operations
RedisString* createRedisString(const char* value);
void freeRedisString(RedisString* str);

// List operations
RedisList* createRedisList(void);
void freeRedisList(RedisList* list);
void listPush(RedisList* list, const char* value, bool head);
char* listPop(RedisList* list, bool head);
char* listIndex(RedisList* list, int64_t index);

// Set operations
RedisSet* createRedisSet(void);
void freeRedisSet(RedisSet* set);
bool setAdd(RedisSet* set, const char* member);
bool setRemove(RedisSet* set, const char* member);
bool setIsMember(RedisSet* set, const char* member);

// Sorted Set operations
RedisSortedSet* createRedisSortedSet(void);
void freeRedisSortedSet(RedisSortedSet* zset);
bool zsetAdd(RedisSortedSet* zset, const char* member, double score);
bool zsetRemove(RedisSortedSet* zset, const char* member);
double zsetScore(RedisSortedSet* zset, const char* member);

// Hash operations
RedisHash* createRedisHash(void);
void freeRedisHash(RedisHash* hash);
bool hashSet(RedisHash* hash, const char* field, const char* value);
char* hashGet(RedisHash* hash, const char* field);
bool hashDelete(RedisHash* hash, const char* field);

// Bitmap operations
RedisBitmap* createRedisBitmap(void);
void freeRedisBitmap(RedisBitmap* bitmap);
bool bitmapSet(RedisBitmap* bitmap, size_t offset, bool value);
bool bitmapGet(RedisBitmap* bitmap, size_t offset);
size_t bitmapCount(RedisBitmap* bitmap);

// HyperLogLog operations
RedisHyperLogLog* createRedisHyperLogLog(void);
void freeRedisHyperLogLog(RedisHyperLogLog* hll);
void hllAdd(RedisHyperLogLog* hll, const char* element);
uint64_t hllCount(RedisHyperLogLog* hll);

// Geo operations
RedisGeo* createRedisGeo(void);
void freeRedisGeo(RedisGeo* geo);
bool geoAdd(RedisGeo* geo, const char* member, double longitude, double latitude);
GeoPoint* geoGet(RedisGeo* geo, const char* member);
double* geoDistance(RedisGeo* geo, const char* member1, const char* member2);

// Stream operations
RedisStream* createRedisStream(void);
void freeRedisStream(RedisStream* stream);
StreamEntry* streamAdd(RedisStream* stream, const char* id, char** fields, char** values, size_t num_fields);
StreamEntry* streamGet(RedisStream* stream, const char* id);
void streamDelete(RedisStream* stream, const char* id);

#endif // REDIS_TYPES_H 