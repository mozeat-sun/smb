/*******************************************************************************
 * Test stubs for remaining socket functionality
 ******************************************************************************/

#include "unity.h"
#include "zoo_socket.h"

// Bind and Listen Tests
void test_zoo_socket_bind_success(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    ZOO_SOCKADDR_UNION addr;
    
    zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &socket_info);
    zoo_sockaddr_any_init(&addr, ZOO_AF_INET, 0);  // Bind to any available port
    
    ZOO_ERROR_TYPE result = zoo_socket_bind(&socket_info, &addr);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_TRUE(socket_info.is_bound);
    
    zoo_socket_close(&socket_info);
}

void test_zoo_socket_bind_invalid_params(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    ZOO_SOCKADDR_UNION addr;
    
    zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &socket_info);
    zoo_sockaddr_any_init(&addr, ZOO_AF_INET, 0);
    
    ZOO_ERROR_TYPE result = zoo_socket_bind(NULL, &addr);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    result = zoo_socket_bind(&socket_info, NULL);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
    
    zoo_socket_close(&socket_info);
}

void test_zoo_socket_listen_success(void) {
    ZOO_SOCKET_INFO_STRUCT socket_info;
    ZOO_SOCKADDR_UNION addr;
    
    zoo_socket_create(ZOO_AF_INET, ZOO_SOCK_STREAM, ZOO_IPPROTO_TCP, &socket_info);
    zoo_sockaddr_any_init(&addr, ZOO_AF_INET, 0);
    zoo_socket_bind(&socket_info, &addr);
    
    ZOO_ERROR_TYPE result = zoo_socket_listen(&socket_info, 5);
    TEST_ASSERT_EQUAL(ZOO_OK, result);
    TEST_ASSERT_TRUE(socket_info.is_listening);
    
    zoo_socket_close(&socket_info);
}

void test_zoo_socket_listen_invalid_params(void) {
    ZOO_ERROR_TYPE result = zoo_socket_listen(NULL, 5);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
}

void test_zoo_socket_accept_invalid_params(void) {
    ZOO_ERROR_TYPE result = zoo_socket_accept(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(ZOO_ERROR_INVALID_PARAM, result);
}
