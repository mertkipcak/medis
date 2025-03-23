#include "../../server/server.h"
#include <string.h>

bool handle_string_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "SET") == 0) {
        if (argc < 3) {
            send_error(client, "ERR wrong number of arguments for SET");
            return false;
        }
        
        RedisString* str = createRedisString(args[2]);
        if (!str) {
            send_error(client, "ERR out of memory");
            return false;
        }
        
        RedisObject* obj = createRedisObject(REDIS_STRING, str);
        if (!obj) {
            freeRedisString(str);
            send_error(client, "ERR out of memory");
            return false;
        }
        
        if (hashmap_put(server->db, key, obj)) {
            send_ok(client);
            return true;
        } else {
            freeRedisObject(obj);
            send_error(client, "ERR failed to set key");
            return false;
        }
    }
    else if (strcasecmp(command, "GET") == 0) {
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_STRING) {
            send_null(client);
            return true;
        }
        
        RedisString* str = obj->data;
        send_string(client, str->value);
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 