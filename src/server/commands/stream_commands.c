#include "../../server/server.h"
#include <string.h>

bool handle_stream_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "XADD") == 0) {
        if (argc < 4 || (argc - 2) % 2 != 0) {
            send_error(client, "ERR wrong number of arguments for XADD");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisStream* stream;
        
        if (!obj) {
            stream = createRedisStream();
            if (!stream) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_STREAM, stream);
            if (!obj) {
                freeRedisStream(stream);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_STREAM) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            stream = obj->data;
        }
        
        // Generate ID if not provided
        char* id = args[2];
        if (strcmp(id, "*") == 0) {
            id = stream_generate_id(stream);
            if (!id) {
                send_error(client, "ERR out of memory");
                return false;
            }
        }
        
        // Add all field-value pairs
        StreamEntry* entry = stream_add(stream, id);
        if (!entry) {
            if (strcmp(id, "*") == 0) {
                free(id);
            }
            send_error(client, "ERR failed to add entry");
            return false;
        }
        
        for (int i = 3; i < argc; i += 2) {
            RedisString* field = createRedisString(args[i]);
            RedisString* value = createRedisString(args[i + 1]);
            
            if (!field || !value) {
                if (field) freeRedisString(field);
                if (value) freeRedisString(value);
                send_error(client, "ERR out of memory");
                return false;
            }
            
            if (!stream_entry_add_field(entry, field, value)) {
                freeRedisString(field);
                freeRedisString(value);
                send_error(client, "ERR failed to add field");
                return false;
            }
        }
        
        if (!hashmap_put(server->db, key, obj)) {
            send_error(client, "ERR failed to update key");
            return false;
        }
        
        send_string(client, id);
        if (strcmp(id, "*") == 0) {
            free(id);
        }
        return true;
    }
    else if (strcasecmp(command, "XRANGE") == 0) {
        if (argc != 4) {
            send_error(client, "ERR wrong number of arguments for XRANGE");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_STREAM) {
            send_array(client, 0);
            return true;
        }
        
        RedisStream* stream = obj->data;
        const char* start = args[2];
        const char* end = args[3];
        
        // Get entries in range
        StreamEntry** entries;
        size_t count;
        if (!stream_range(stream, start, end, &entries, &count)) {
            send_error(client, "ERR failed to get range");
            return false;
        }
        
        send_array(client, count);
        
        for (size_t i = 0; i < count; i++) {
            StreamEntry* entry = entries[i];
            send_array(client, 2);
            send_string(client, entry->id);
            
            // Send fields
            send_array(client, entry->field_count * 2);
            for (size_t j = 0; j < entry->field_count; j++) {
                send_string(client, entry->fields[j]->field->value);
                send_string(client, entry->fields[j]->value->value);
            }
        }
        
        free(entries);
        return true;
    }
    else if (strcasecmp(command, "XREAD") == 0) {
        if (argc < 3) {
            send_error(client, "ERR wrong number of arguments for XREAD");
            return false;
        }
        
        // Parse streams and IDs
        size_t stream_count = (argc - 2) / 2;
        if ((argc - 2) % 2 != 0) {
            send_error(client, "ERR wrong number of arguments for XREAD");
            return false;
        }
        
        send_array(client, 1);
        send_array(client, stream_count);
        
        for (size_t i = 0; i < stream_count; i++) {
            const char* stream_key = args[2 + i * 2];
            const char* id = args[3 + i * 2];
            
            RedisObject* obj = hashmap_get(server->db, stream_key);
            if (!obj || obj->type != REDIS_STREAM) {
                send_array(client, 2);
                send_string(client, stream_key);
                send_array(client, 0);
                continue;
            }
            
            RedisStream* stream = obj->data;
            StreamEntry** entries;
            size_t count;
            
            if (!stream_read(stream, id, &entries, &count)) {
                send_error(client, "ERR failed to read stream");
                return false;
            }
            
            send_array(client, 2);
            send_string(client, stream_key);
            send_array(client, count);
            
            for (size_t j = 0; j < count; j++) {
                StreamEntry* entry = entries[j];
                send_array(client, 2);
                send_string(client, entry->id);
                
                // Send fields
                send_array(client, entry->field_count * 2);
                for (size_t k = 0; k < entry->field_count; k++) {
                    send_string(client, entry->fields[k]->field->value);
                    send_string(client, entry->fields[k]->value->value);
                }
            }
            
            free(entries);
        }
        
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 