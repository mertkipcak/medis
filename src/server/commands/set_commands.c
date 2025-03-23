#include "../../server/server.h"
#include <string.h>

bool handle_set_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "SADD") == 0) {
        if (argc < 3) {
            send_error(client, "ERR wrong number of arguments for SADD");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisSet* set;
        
        if (!obj) {
            set = createRedisSet();
            if (!set) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_SET, set);
            if (!obj) {
                freeRedisSet(set);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_SET) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            set = obj->data;
        }
        
        // Add all arguments to the set
        size_t added = 0;
        for (int i = 2; i < argc; i++) {
            RedisString* str = createRedisString(args[i]);
            if (!str) {
                send_error(client, "ERR out of memory");
                return false;
            }
            if (set_add(set, str)) {
                added++;
            } else {
                freeRedisString(str);
            }
        }
        
        if (!hashmap_put(server->db, key, obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_integer(client, added);
        return true;
    }
    else if (strcasecmp(command, "SMEMBERS") == 0) {
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_SET) {
            send_array(client, 0);
            return true;
        }
        
        RedisSet* set = obj->data;
        send_array(client, set->size);
        
        // Iterate through all members
        for (size_t i = 0; i < set->capacity; i++) {
            SetNode* node = set->buckets[i];
            while (node) {
                send_string(client, node->value->value);
                node = node->next;
            }
        }
        
        return true;
    }
    else if (strcasecmp(command, "SISMEMBER") == 0) {
        if (argc != 3) {
            send_error(client, "ERR wrong number of arguments for SISMEMBER");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_SET) {
            send_integer(client, 0);
            return true;
        }
        
        RedisSet* set = obj->data;
        RedisString* str = createRedisString(args[2]);
        if (!str) {
            send_error(client, "ERR out of memory");
            return false;
        }
        
        bool is_member = set_contains(set, str);
        freeRedisString(str);
        
        send_integer(client, is_member ? 1 : 0);
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 