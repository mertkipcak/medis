#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "test_hashmap.h"
#include "test_redis_server.h"

int main(void) {
    // Initialize CUnit test registry
    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    // Add test suites
    if (init_hashmap_suite() != CUE_SUCCESS ||
        init_redis_server_suite() != CUE_SUCCESS) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Run all tests using the basic interface
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Clean up
    CU_cleanup_registry();
    return CU_get_error();
} 