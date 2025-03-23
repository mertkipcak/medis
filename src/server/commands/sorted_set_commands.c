#include "../../server/server.h"
#include <string.h>
#include <stdlib.h>

bool handle_sorted_set_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "ZADD") == 0) {
        if (argc < 4 || (argc - 2) % 2 != 0) {
            send_error(client, "ERR wrong number of arguments for ZADD");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisSortedSet* zset;
        
        if (!obj) {
            zset = createRedisSortedSet();
            if (!zset) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_SORTED_SET, zset);
            if (!obj) {
                freeRedisSortedSet(zset);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_SORTED_SET) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            zset = obj->data;
        }
        
        // Add all score-member pairs
        size_t added = 0;
        for (int i = 2; i < argc; i += 2) {
            double score = strtod(args[i], NULL);
            RedisString* member = createRedisString(args[i + 1]);
            if (!member) {
                send_error(client, "ERR out of memory");
                return false;
            }
            
            if (sorted_set_add(zset, member, score)) {
                added++;
            } else {
                freeRedisString(member);
            }
        }
        
        if (!hashmap_put(server->db, key, obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_integer(client, added);
        return true;
    }
    else if (strcasecmp(command, "ZRANGE") == 0) {
        if (argc != 4 && argc != 5) {
            send_error(client, "ERR wrong number of arguments for ZRANGE");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_SORTED_SET) {
            send_array(client, 0);
            return true;
        }
        
        RedisSortedSet* zset = obj->data;
        long start = strtol(args[2], NULL, 10);
        long end = strtol(args[3], NULL, 10);
        bool withscores = argc == 5 && strcasecmp(args[4], "WITHSCORES") == 0;
        
        // Handle negative indices
        if (start < 0) start = zset->size + start;
        if (end < 0) end = zset->size + end;
        
        // Bounds checking
        if (start < 0) start = 0;
        if (end >= zset->size) end = zset->size - 1;
        if (start > end) {
            send_array(client, 0);
            return true;
        }
        
        // Create response array
        size_t count = end - start + 1;
        send_array(client, withscores ? count * 2 : count);
        
        SortedSetNode* current = zset->head;
        size_t current_pos = 0;
        
        while (current && current_pos < start) {
            current = current->next;
            current_pos++;
        }
        
        while (current && current_pos <= end) {
            send_string(client, current->member->value);
            if (withscores) {
                char score_str[32];
                snprintf(score_str, sizeof(score_str), "%.17g", current->score);
                send_string(client, score_str);
            }
            current = current->next;
            current_pos++;
        }
        
        return true;
    }
    else if (strcasecmp(command, "ZSCORE") == 0) {
        if (argc != 3) {
            send_error(client, "ERR wrong number of arguments for ZSCORE");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_SORTED_SET) {
            send_null(client);
            return true;
        }
        
        RedisSortedSet* zset = obj->data;
        RedisString* member = createRedisString(args[2]);
        if (!member) {
            send_error(client, "ERR out of memory");
            return false;
        }
        
        double score;
        if (sorted_set_get_score(zset, member, &score)) {
            char score_str[32];
            snprintf(score_str, sizeof(score_str), "%.17g", score);
            send_string(client, score_str);
        } else {
            send_null(client);
        }
        
        freeRedisString(member);
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 