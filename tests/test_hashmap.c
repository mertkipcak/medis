#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>
#include "../src/hashmap/hashmap.h"

// Test fixtures
static HashMap* map;

// Setup and teardown functions
static int setup(void) {
    map = createHashMap();
    return (map != NULL) ? 0 : -1;
}

static int teardown(void) {
    if (map) {
        freeHashMap(map);
        map = NULL;
    }
    return 0;
}

// Test cases
static void test_create_hashmap(void) {
    CU_ASSERT_PTR_NOT_NULL(map);
    CU_ASSERT_EQUAL(map->size, INITIAL_HASHMAP_SIZE);
    CU_ASSERT_EQUAL(map->itemCount, 0);
}

static void test_insert_and_get(void) {
    const char* key = "test_key";
    unsigned char value[] = "test_value";
    
    // Test insertion
    hashMapInsert(map, key, value, sizeof(value));
    CU_ASSERT_EQUAL(map->itemCount, 1);
    
    // Test retrieval
    size_t foundSize;
    unsigned char* retrieved = hashMapGet(map, key, &foundSize);
    CU_ASSERT_PTR_NOT_NULL(retrieved);
    CU_ASSERT_EQUAL(foundSize, sizeof(value));
    CU_ASSERT_STRING_EQUAL((char*)retrieved, (char*)value);
}

static void test_remove(void) {
    const char* key = "test_key";
    unsigned char value[] = "test_value";
    
    // Insert a value
    hashMapInsert(map, key, value, sizeof(value));
    CU_ASSERT_EQUAL(map->itemCount, 1);
    
    // Remove the value
    hashMapRemove(map, key);
    CU_ASSERT_EQUAL(map->itemCount, 0);
    
    // Verify removal
    size_t foundSize;
    unsigned char* retrieved = hashMapGet(map, key, &foundSize);
    CU_ASSERT_PTR_NULL(retrieved);
}

static void test_collision_handling(void) {
    // Insert two values that should hash to the same bucket
    const char* key1 = "key1";
    const char* key2 = "key1";  // Same key to force collision
    unsigned char value1[] = "value1";
    unsigned char value2[] = "value2";
    
    hashMapInsert(map, key1, value1, sizeof(value1));
    hashMapInsert(map, key2, value2, sizeof(value2));
    
    // Verify both values can be retrieved
    size_t foundSize;
    unsigned char* retrieved1 = hashMapGet(map, key1, &foundSize);
    unsigned char* retrieved2 = hashMapGet(map, key2, &foundSize);
    
    CU_ASSERT_PTR_NOT_NULL(retrieved1);
    CU_ASSERT_PTR_NOT_NULL(retrieved2);
    CU_ASSERT_STRING_EQUAL((char*)retrieved1, (char*)value1);
    CU_ASSERT_STRING_EQUAL((char*)retrieved2, (char*)value2);
}

static void test_resize(void) {
    // Insert enough items to trigger resize
    for (int i = 0; i < INITIAL_HASHMAP_SIZE * RESIZE_UP_THRESHOLD + 1; i++) {
        char key[16];
        char value[16];
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(value, sizeof(value), "value%d", i);
        hashMapInsert(map, key, (unsigned char*)value, strlen(value) + 1);
    }
    
    // Verify resize occurred
    CU_ASSERT(map->size > INITIAL_HASHMAP_SIZE);
    
    // Verify all values can still be retrieved
    for (int i = 0; i < INITIAL_HASHMAP_SIZE * RESIZE_UP_THRESHOLD + 1; i++) {
        char key[16];
        char expected_value[16];
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(expected_value, sizeof(expected_value), "value%d", i);
        
        size_t foundSize;
        unsigned char* retrieved = hashMapGet(map, key, &foundSize);
        CU_ASSERT_PTR_NOT_NULL(retrieved);
        CU_ASSERT_STRING_EQUAL((char*)retrieved, expected_value);
    }
}

// Test suite initialization
int init_hashmap_suite(void) {
    CU_pSuite suite = CU_add_suite("HashMap Tests", setup, teardown);
    if (!suite) return CU_get_error();
    
    // Add test cases
    if (!CU_add_test(suite, "test_create_hashmap", test_create_hashmap) ||
        !CU_add_test(suite, "test_insert_and_get", test_insert_and_get) ||
        !CU_add_test(suite, "test_remove", test_remove) ||
        !CU_add_test(suite, "test_collision_handling", test_collision_handling) ||
        !CU_add_test(suite, "test_resize", test_resize)) {
        return CU_get_error();
    }
    
    return CUE_SUCCESS;
} 