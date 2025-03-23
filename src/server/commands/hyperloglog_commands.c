#include "../../server/server.h"
#include <string.h>

bool handle_hyperloglog_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "PFADD") == 0) {
        if (argc < 3) {
            send_error(client, "ERR wrong number of arguments for PFADD");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisHyperLogLog* hll;
        
        if (!obj) {
            hll = createRedisHyperLogLog();
            if (!hll) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_HYPERLOGLOG, hll);
            if (!obj) {
                freeRedisHyperLogLog(hll);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_HYPERLOGLOG) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            hll = obj->data;
        }
        
        // Add all elements
        bool changed = false;
        for (int i = 2; i < argc; i++) {
            RedisString* element = createRedisString(args[i]);
            if (!element) {
                send_error(client, "ERR out of memory");
                return false;
            }
            
            if (hyperloglog_add(hll, element)) {
                changed = true;
            }
            freeRedisString(element);
        }
        
        if (!hashmap_put(server->db, key, obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_integer(client, changed ? 1 : 0);
        return true;
    }
    else if (strcasecmp(command, "PFCOUNT") == 0) {
        if (argc < 2) {
            send_error(client, "ERR wrong number of arguments for PFCOUNT");
            return false;
        }
        
        if (argc == 2) {
            RedisObject* obj = hashmap_get(server->db, key);
            if (!obj || obj->type != REDIS_HYPERLOGLOG) {
                send_integer(client, 0);
                return true;
            }
            
            RedisHyperLogLog* hll = obj->data;
            size_t count = hyperloglog_count(hll);
            send_integer(client, count);
            return true;
        }
        else {
            // Merge multiple HLLs
            RedisHyperLogLog* merged = createRedisHyperLogLog();
            if (!merged) {
                send_error(client, "ERR out of memory");
                return false;
            }
            
            bool error = false;
            for (int i = 1; i < argc; i++) {
                RedisObject* obj = hashmap_get(server->db, args[i]);
                if (!obj || obj->type != REDIS_HYPERLOGLOG) {
                    error = true;
                    break;
                }
                
                RedisHyperLogLog* hll = obj->data;
                if (!hyperloglog_merge(merged, hll)) {
                    error = true;
                    break;
                }
            }
            
            if (error) {
                freeRedisHyperLogLog(merged);
                send_error(client, "ERR invalid HyperLogLog key");
                return false;
            }
            
            size_t count = hyperloglog_count(merged);
            freeRedisHyperLogLog(merged);
            
            send_integer(client, count);
            return true;
        }
    }
    else if (strcasecmp(command, "PFMERGE") == 0) {
        if (argc < 3) {
            send_error(client, "ERR wrong number of arguments for PFMERGE");
            return false;
        }
        
        RedisObject* dest_obj = hashmap_get(server->db, key);
        RedisHyperLogLog* dest;
        
        if (!dest_obj) {
            dest = createRedisHyperLogLog();
            if (!dest) {
                send_error(client, "ERR out of memory");
                return false;
            }
            dest_obj = createRedisObject(REDIS_HYPERLOGLOG, dest);
            if (!dest_obj) {
                freeRedisHyperLogLog(dest);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (dest_obj->type != REDIS_HYPERLOGLOG) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            dest = dest_obj->data;
        }
        
        // Merge all source HLLs
        for (int i = 2; i < argc; i++) {
            RedisObject* src_obj = hashmap_get(server->db, args[i]);
            if (!src_obj || src_obj->type != REDIS_HYPERLOGLOG) {
                send_error(client, "ERR invalid HyperLogLog key");
                return false;
            }
            
            RedisHyperLogLog* src = src_obj->data;
            if (!hyperloglog_merge(dest, src)) {
                send_error(client, "ERR failed to merge HyperLogLogs");
                return false;
            }
        }
        
        if (!hashmap_put(server->db, key, dest_obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_ok(client);
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 