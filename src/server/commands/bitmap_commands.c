#include "../../server/server.h"
#include <string.h>

bool handle_bitmap_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "SETBIT") == 0) {
        if (argc != 4) {
            send_error(client, "ERR wrong number of arguments for SETBIT");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisBitmap* bitmap;
        
        if (!obj) {
            bitmap = createRedisBitmap();
            if (!bitmap) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_BITMAP, bitmap);
            if (!obj) {
                freeRedisBitmap(bitmap);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_BITMAP) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            bitmap = obj->data;
        }
        
        size_t offset = strtoul(args[2], NULL, 10);
        int value = strtol(args[3], NULL, 10);
        
        if (value != 0 && value != 1) {
            send_error(client, "ERR bit is not an integer or out of range");
            return false;
        }
        
        bool old_value = bitmap_set(bitmap, offset, value == 1);
        
        if (!hashmap_put(server->db, key, obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_integer(client, old_value ? 1 : 0);
        return true;
    }
    else if (strcasecmp(command, "GETBIT") == 0) {
        if (argc != 3) {
            send_error(client, "ERR wrong number of arguments for GETBIT");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_BITMAP) {
            send_integer(client, 0);
            return true;
        }
        
        RedisBitmap* bitmap = obj->data;
        size_t offset = strtoul(args[2], NULL, 10);
        
        bool value = bitmap_get(bitmap, offset);
        send_integer(client, value ? 1 : 0);
        return true;
    }
    else if (strcasecmp(command, "BITCOUNT") == 0) {
        if (argc != 2 && argc != 4) {
            send_error(client, "ERR wrong number of arguments for BITCOUNT");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_BITMAP) {
            send_integer(client, 0);
            return true;
        }
        
        RedisBitmap* bitmap = obj->data;
        size_t start = 0;
        size_t end = bitmap->size * 8 - 1;
        
        if (argc == 4) {
            start = strtoul(args[2], NULL, 10);
            end = strtoul(args[3], NULL, 10);
        }
        
        size_t count = bitmap_count(bitmap, start, end);
        send_integer(client, count);
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 