#include "../../server/server.h"
#include <string.h>

bool handle_list_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "LPUSH") == 0) {
        if (argc < 3) {
            send_error(client, "ERR wrong number of arguments for LPUSH");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisList* list;
        
        if (!obj) {
            list = createRedisList();
            if (!list) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_LIST, list);
            if (!obj) {
                freeRedisList(list);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_LIST) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            list = obj->data;
        }
        
        // Push all arguments to the list
        for (int i = 2; i < argc; i++) {
            RedisString* str = createRedisString(args[i]);
            if (!str) {
                send_error(client, "ERR out of memory");
                return false;
            }
            list_push_front(list, str);
        }
        
        if (!hashmap_put(server->db, key, obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_integer(client, list->size);
        return true;
    }
    else if (strcasecmp(command, "RPUSH") == 0) {
        if (argc < 3) {
            send_error(client, "ERR wrong number of arguments for RPUSH");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisList* list;
        
        if (!obj) {
            list = createRedisList();
            if (!list) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_LIST, list);
            if (!obj) {
                freeRedisList(list);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_LIST) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            list = obj->data;
        }
        
        // Push all arguments to the list
        for (int i = 2; i < argc; i++) {
            RedisString* str = createRedisString(args[i]);
            if (!str) {
                send_error(client, "ERR out of memory");
                return false;
            }
            list_push_back(list, str);
        }
        
        if (!hashmap_put(server->db, key, obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_integer(client, list->size);
        return true;
    }
    else if (strcasecmp(command, "LRANGE") == 0) {
        if (argc != 4) {
            send_error(client, "ERR wrong number of arguments for LRANGE");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_LIST) {
            send_array(client, 0);
            return true;
        }
        
        RedisList* list = obj->data;
        long start = strtol(args[2], NULL, 10);
        long end = strtol(args[3], NULL, 10);
        
        // Handle negative indices
        if (start < 0) start = list->size + start;
        if (end < 0) end = list->size + end;
        
        // Bounds checking
        if (start < 0) start = 0;
        if (end >= list->size) end = list->size - 1;
        if (start > end) {
            send_array(client, 0);
            return true;
        }
        
        // Create response array
        size_t count = end - start + 1;
        send_array(client, count);
        
        ListNode* current = list->head;
        size_t current_pos = 0;
        
        while (current && current_pos < start) {
            current = current->next;
            current_pos++;
        }
        
        while (current && current_pos <= end) {
            send_string(client, current->value->value);
            current = current->next;
            current_pos++;
        }
        
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 