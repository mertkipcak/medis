#include "../../server/server.h"
#include <string.h>
#include <math.h>

bool handle_geo_command(Server* server, Client* client, const char* command, char** args, int argc) {
    if (!server || !client || !command || !args || argc < 2) {
        send_error(client, "ERR wrong number of arguments");
        return false;
    }
    
    const char* key = args[1];
    
    if (strcasecmp(command, "GEOADD") == 0) {
        if (argc < 5 || (argc - 2) % 3 != 0) {
            send_error(client, "ERR wrong number of arguments for GEOADD");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        RedisGeo* geo;
        
        if (!obj) {
            geo = createRedisGeo();
            if (!geo) {
                send_error(client, "ERR out of memory");
                return false;
            }
            obj = createRedisObject(REDIS_GEO, geo);
            if (!obj) {
                freeRedisGeo(geo);
                send_error(client, "ERR out of memory");
                return false;
            }
        } else if (obj->type != REDIS_GEO) {
            send_error(client, "WRONGTYPE Operation against a key holding the wrong kind of value");
            return false;
        } else {
            geo = obj->data;
        }
        
        // Add all location-member pairs
        size_t added = 0;
        for (int i = 2; i < argc; i += 3) {
            double longitude = strtod(args[i], NULL);
            double latitude = strtod(args[i + 1], NULL);
            
            if (longitude < -180 || longitude > 180 || latitude < -85.05112878 || latitude > 85.05112878) {
                send_error(client, "ERR invalid coordinates");
                return false;
            }
            
            RedisString* member = createRedisString(args[i + 2]);
            if (!member) {
                send_error(client, "ERR out of memory");
                return false;
            }
            
            if (geo_add(geo, member, longitude, latitude)) {
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
    else if (strcasecmp(command, "GEOPOS") == 0) {
        if (argc < 3) {
            send_error(client, "ERR wrong number of arguments for GEOPOS");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_GEO) {
            send_array(client, argc - 2);
            for (int i = 2; i < argc; i++) {
                send_null(client);
            }
            return true;
        }
        
        RedisGeo* geo = obj->data;
        send_array(client, argc - 2);
        
        for (int i = 2; i < argc; i++) {
            RedisString* member = createRedisString(args[i]);
            if (!member) {
                send_null(client);
                continue;
            }
            
            double longitude, latitude;
            if (geo_get(geo, member, &longitude, &latitude)) {
                send_array(client, 2);
                char lon_str[32], lat_str[32];
                snprintf(lon_str, sizeof(lon_str), "%.17g", longitude);
                snprintf(lat_str, sizeof(lat_str), "%.17g", latitude);
                send_string(client, lon_str);
                send_string(client, lat_str);
            } else {
                send_null(client);
            }
            
            freeRedisString(member);
        }
        
        return true;
    }
    else if (strcasecmp(command, "GEODIST") == 0) {
        if (argc != 3 && argc != 4) {
            send_error(client, "ERR wrong number of arguments for GEODIST");
            return false;
        }
        
        RedisObject* obj = hashmap_get(server->db, key);
        if (!obj || obj->type != REDIS_GEO) {
            send_null(client);
            return true;
        }
        
        RedisGeo* geo = obj->data;
        RedisString* member1 = createRedisString(args[2]);
        RedisString* member2 = createRedisString(args[3]);
        
        if (!member1 || !member2) {
            if (member1) freeRedisString(member1);
            if (member2) freeRedisString(member2);
            send_error(client, "ERR out of memory");
            return false;
        }
        
        double distance;
        if (geo_dist(geo, member1, member2, &distance)) {
            char dist_str[32];
            snprintf(dist_str, sizeof(dist_str), "%.17g", distance);
            send_string(client, dist_str);
        } else {
            send_null(client);
        }
        
        freeRedisString(member1);
        freeRedisString(member2);
        return true;
    }
    
    send_error(client, "ERR unknown command");
    return false;
} 