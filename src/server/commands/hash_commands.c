#include "../../server/server.h"
#include <string.h>

bool handle_hash_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "HSET") == 0) {
        if (argc < 4 || (argc - 2) % 2 != 0) {
            send_error(client, "ERR wrong number of arguments for HSET");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisHash* hash;
        
        if (!obj) {
            hash = createRedisHash();
            if (!hash) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_HASH, hash);
            if (!obj) {
                freeRedisHash(hash);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_HASH) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            hash = obj->data;
        }
        
        // Add all field-value pairs
        size_t added = 0;
        for (int i = 2; i < argc; i += 2) {
            RedisString* field = createRedisString(args[i]);
            RedisString* value = createRedisString(args[i + 1]);
            
            if (!field || !value) {
                if (field) freeRedisString(field);
                if (value) freeRedisString(value);
                send_error(client, "ERR out of memory");
                return false;
            }
            
            if (hash_set(hash, field, value)) {
                added++;
            } else {
                freeRedisString(field);
                freeRedisString(value);
            }
        }
        
        if (!hashmap_put(server->db, key, obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_integer(client, added);
        return true;
    }
    else if (strcasecmp(command, "HGET") == 0) {
        if (argc != 3) {
            send_error(client, "ERR wrong number of arguments for HGET");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_HASH) {
            send_null(client);
            return true;
        }
        
        RedisHash* hash = obj->data;
        RedisString* field = createRedisString(args[2]);
        if (!field) {
            send_error(client, "ERR out of memory");
            return false;
        }
        
        RedisString* value = hash_get(hash, field);
        freeRedisString(field);
        
        if (value) {
            send_string(client, value->value);
        } else {
            send_null(client);
        }
        
        return true;
    }
    else if (strcasecmp(command, "HGETALL") == 0) {
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_HASH) {
            send_array(client, 0);
            return true;
        }
        
        RedisHash* hash = obj->data;
        send_array(client, hash->size * 2);
        
        // Iterate through all field-value pairs
        for (size_t i = 0; i < hash->capacity; i++) {
            HashNode* node = hash->buckets[i];
            while (node) {
                send_string(client, node->field->value);
                send_string(client, node->value->value);
                node = node->next;
            }
        }
        
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 