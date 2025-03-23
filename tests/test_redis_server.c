#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../src/server/redis_server.h"

// Test fixtures
static RedisServer* server;
static int test_port = 6380;  // Use different port than default for testing

// Setup and teardown functions
static int setup(void) {
    server = createRedisServer(test_port);
    return (server != NULL) ? 0 : -1;
}

static int teardown(void) {
    if (server) {
        stopRedisServer(server);
        freeRedisServer(server);
        server = NULL;
    }
    return 0;
}

// Helper function to create a test client
static int create_test_client(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(test_port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

// Helper function to send RESP command
static void send_resp_command(int sock, const char* cmd) {
    char* resp = malloc(strlen(cmd) * 2 + 16);  // Extra space for RESP formatting
    if (!resp) return;

    snprintf(resp, strlen(cmd) * 2 + 16, "*1\r\n$%zu\r\n%s\r\n", strlen(cmd), cmd);
    send(sock, resp, strlen(resp), 0);
    free(resp);
}

// Test cases
static void test_server_creation(void) {
    CU_ASSERT_PTR_NOT_NULL(server);
    CU_ASSERT_PTR_NOT_NULL(server->db);
    CU_ASSERT_EQUAL(server->server_socket, -1);  // Not started yet
}

static void test_set_command(void) {
    int client_sock = create_test_client();
    CU_ASSERT(client_sock > 0);

    // Send SET command
    send_resp_command(client_sock, "SET test_key test_value");

    // Read response
    char buffer[1024];
    ssize_t bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    CU_ASSERT(bytes > 0);
    buffer[bytes] = '\0';

    // Verify response
    CU_ASSERT_STRING_EQUAL(buffer, "$2\r\nOK\r\n");

    close(client_sock);
}

static void test_get_command(void) {
    int client_sock = create_test_client();
    CU_ASSERT(client_sock > 0);

    // First set a value
    send_resp_command(client_sock, "SET test_key test_value");

    // Then get it
    send_resp_command(client_sock, "GET test_key");

    // Read response
    char buffer[1024];
    ssize_t bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    CU_ASSERT(bytes > 0);
    buffer[bytes] = '\0';

    // Verify response
    CU_ASSERT_STRING_EQUAL(buffer, "$11\r\ntest_value\r\n");

    close(client_sock);
}

static void test_del_command(void) {
    int client_sock = create_test_client();
    CU_ASSERT(client_sock > 0);

    // First set a value
    send_resp_command(client_sock, "SET test_key test_value");

    // Then delete it
    send_resp_command(client_sock, "DEL test_key");

    // Read response
    char buffer[1024];
    ssize_t bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    CU_ASSERT(bytes > 0);
    buffer[bytes] = '\0';

    // Verify response
    CU_ASSERT_STRING_EQUAL(buffer, "$1\r\n1\r\n");

    // Try to get deleted key
    send_resp_command(client_sock, "GET test_key");
    bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    CU_ASSERT(bytes > 0);
    buffer[bytes] = '\0';

    // Verify response
    CU_ASSERT_STRING_EQUAL(buffer, "$5\r\n(nil)\r\n");

    close(client_sock);
}

// Test suite initialization
int init_redis_server_suite(void) {
    CU_pSuite suite = CU_add_suite("Redis Server Tests", setup, teardown);
    if (!suite) return CU_get_error();
    
    // Add test cases
    if (!CU_add_test(suite, "test_server_creation", test_server_creation) ||
        !CU_add_test(suite, "test_set_command", test_set_command) ||
        !CU_add_test(suite, "test_get_command", test_get_command) ||
        !CU_add_test(suite, "test_del_command", test_del_command)) {
        return CU_get_error();
    }
    
    return CUE_SUCCESS;
} 