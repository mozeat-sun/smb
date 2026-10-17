/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_zoo_smb_protocol_
 * File name: test_zoo_smb_protocol.c
 * Description: Unity-based protocol tests (converted from GTest)
 * Traceability coverage:
 * - REQ-COMP-001: explicit wire header magic/version parsing and validation.
 * - REQ-SAFE-003: requirement-linked unit evidence for protocol encoding/decoding.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                converted from GTest
 ******************************************************************************/

#include "test_smb_helpers.h"
#include "zoo_smb_protocol.h"
#include <string.h>

void setUp(void)
{
    // Setup memory pool for tests
    test_setup_memory_pool();
}

void tearDown(void)
{
    // Cleanup after tests
    test_teardown_memory_pool();
}

void test_make_and_parse_discovery_payload(void)
{
    ZOO_SMB_MSG_STRUCT msg = {0};
    const char* service = "svc";
    const char* addr = "127.0.0.1";
    uint16_t port = 12345;
    const char* topic = "t1";
    ZOO_SMB_TRANSPORT_TYPE_ENUM type = ZOO_SMB_TRANSPORT_TYPE_SHM;

    zoo_smb_protocol_make_discovery_payload_message(&msg, service, addr, port, topic, type);

    TEST_ASSERT_NOT_NULL(msg.payload);
    TEST_ASSERT_GREATER_THAN_UINT32(0, msg.header.payload_size);

    char parsed_service[32] = {0};
    char parsed_addr[32] = {0};
    char parsed_topic[32] = {0};
    uint16_t parsed_port = 0;
    ZOO_SMB_TRANSPORT_TYPE_ENUM parsed_type = (ZOO_SMB_TRANSPORT_TYPE_ENUM)0;

    ZOO_BOOL ok = zoo_smb_protocol_parse_discovery_payload_message(
        &msg, parsed_service, parsed_addr, &parsed_port, parsed_topic, &parsed_type);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_STREQ(service, parsed_service);
    TEST_ASSERT_STREQ(addr, parsed_addr);
    TEST_ASSERT_STREQ(topic, parsed_topic);
    TEST_ASSERT_EQUAL_UINT16(port, parsed_port);
    TEST_ASSERT_EQUAL_INT(type, parsed_type);
}

void test_header_validation(void)
{
    ZOO_SMB_MSG_HEADER_STRUCT header = {0};
    header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    header.version = ZOO_SMB_MSG_VERSION;
    header.msg_type = ZOO_SMB_MSG_TYPE_PUB;
    header.payload_size = 100;
    
    ZOO_BOOL is_valid = zoo_smb_protocol_is_header_valid(&header);
    TEST_ASSERT_TRUE(is_valid);
}

void test_invalid_header_magic(void)
{
    ZOO_SMB_MSG_HEADER_STRUCT header = {0};
    header.magic = 0xDEADBEEF; // Wrong magic number
    header.version = ZOO_SMB_MSG_VERSION;
    header.msg_type = ZOO_SMB_MSG_TYPE_PUB;
    header.payload_size = 100;
    
    ZOO_BOOL is_valid = zoo_smb_protocol_is_header_valid(&header);
    TEST_ASSERT_FALSE(is_valid);
}

void test_invalid_header_version(void)
{
    ZOO_SMB_MSG_HEADER_STRUCT header = {0};
    header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    header.version = 999; // Invalid version
    header.msg_type = ZOO_SMB_MSG_TYPE_PUB;
    header.payload_size = 100;
    
    ZOO_BOOL is_valid = zoo_smb_protocol_is_header_valid(&header);
    TEST_ASSERT_FALSE(is_valid);
}

void test_protocol_constants(void)
{
    // Test that protocol constants are defined
    TEST_ASSERT_NOT_EQUAL_UINT32(0, ZOO_SMB_MSG_MAGIC_NUMBER);
    TEST_ASSERT_NOT_EQUAL_UINT32(0, ZOO_SMB_MSG_VERSION);
}

void test_transport_type_validation(void)
{
    // Test valid transport types
    TEST_ASSERT_TRUE(ZOO_SMB_TRANSPORT_TYPE_UDP >= 0);
    TEST_ASSERT_TRUE(ZOO_SMB_TRANSPORT_TYPE_TCP >= 0);
    TEST_ASSERT_TRUE(ZOO_SMB_TRANSPORT_TYPE_SHM >= 0);
}

void test_discovery_payload_ex2_round_trip_with_metadata(void)
{
    ZOO_SMB_MSG_STRUCT msg = {0};
    ZOO_BOOL made = zoo_smb_protocol_make_discovery_payload_message_ex2(
        &msg,
        "svc_meta",
        "127.0.0.1",
        23456,
        "meta/topic",
        ZOO_SMB_TRANSPORT_TYPE_TCP,
        "239.0.0.1",
        10000,
        "127.0.0.1",
        ZOO_SMB_SERVICE_CAPABILITY_SHM | ZOO_SMB_SERVICE_CAPABILITY_TCP,
        "svc_meta_endpoint");
    TEST_ASSERT_TRUE(made);

    char service[32] = {0};
    char address[32] = {0};
    char topic[32] = {0};
    char multicast[32] = {0};
    char host_fingerprint[64] = {0};
    char shm_endpoint[64] = {0};
    uint16_t port = 0;
    uint16_t multicast_port = 0;
    uint32_t capabilities = 0;
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport = ZOO_SMB_TRANSPORT_TYPE_MIN;

    ZOO_BOOL parsed = zoo_smb_protocol_parse_discovery_payload_message_ex2(
        &msg,
        service,
        address,
        &port,
        topic,
        &transport,
        multicast,
        &multicast_port,
        host_fingerprint,
        &capabilities,
        shm_endpoint);

    TEST_ASSERT_TRUE(parsed);
    TEST_ASSERT_STREQ("svc_meta", service);
    TEST_ASSERT_STREQ("127.0.0.1", address);
    TEST_ASSERT_STREQ("meta/topic", topic);
    TEST_ASSERT_EQUAL_UINT16(23456, port);
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_TCP, transport);
    TEST_ASSERT_STREQ("239.0.0.1", multicast);
    TEST_ASSERT_EQUAL_UINT16(10000, multicast_port);
    TEST_ASSERT_STREQ("127.0.0.1", host_fingerprint);
    TEST_ASSERT_EQUAL_UINT32(ZOO_SMB_SERVICE_CAPABILITY_SHM | ZOO_SMB_SERVICE_CAPABILITY_TCP, capabilities);
    TEST_ASSERT_STREQ("svc_meta_endpoint", shm_endpoint);
}

void test_discovery_payload_ex2_parses_legacy_token_order(void)
{
    ZOO_SMB_MSG_STRUCT msg = {0};
    const char legacy_payload[] = "svc_legacy t1 127.0.0.1:12345 0:0 3";

    msg.payload = zoo_allocate_from_pool(sizeof(legacy_payload));
    TEST_ASSERT_NOT_NULL(msg.payload);
    memcpy(msg.payload, legacy_payload, sizeof(legacy_payload));
    msg.header.payload_size = (uint32_t)(sizeof(legacy_payload) - 1U);

    char service[32] = {0};
    char address[32] = {0};
    char topic[32] = {0};
    char multicast[32] = {0};
    uint16_t port = 0;
    uint16_t multicast_port = 0;
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport = ZOO_SMB_TRANSPORT_TYPE_MIN;

    ZOO_BOOL parsed = zoo_smb_protocol_parse_discovery_payload_message_ex2(
        &msg,
        service,
        address,
        &port,
        topic,
        &transport,
        multicast,
        &multicast_port,
        NULL,
        NULL,
        NULL);

    TEST_ASSERT_TRUE(parsed);
    TEST_ASSERT_STREQ("svc_legacy", service);
    TEST_ASSERT_STREQ("127.0.0.1", address);
    TEST_ASSERT_STREQ("t1", topic);
    TEST_ASSERT_EQUAL_UINT16(12345, port);
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_TCP, transport);
    TEST_ASSERT_STREQ("0", multicast);
    TEST_ASSERT_EQUAL_UINT16(0, multicast_port);
}

void test_discovery_payload_ex2_rejects_unsafe_metadata_values(void)
{
    ZOO_SMB_MSG_STRUCT msg = {0};
    const char payload[] =
        "svc_meta t1 127.0.0.1:12345 3 239.0.0.1:9000 hf=127.0.0.1$bad cap=3 shm=svc_meta";

    msg.payload = zoo_allocate_from_pool(sizeof(payload));
    TEST_ASSERT_NOT_NULL(msg.payload);
    memcpy(msg.payload, payload, sizeof(payload));
    msg.header.payload_size = (uint32_t)(sizeof(payload) - 1U);

    char service[32] = {0};
    char address[32] = {0};
    char topic[32] = {0};
    char multicast[32] = {0};
    char host_fingerprint[64] = {0};
    char shm_endpoint[64] = {0};
    uint16_t port = 0;
    uint16_t multicast_port = 0;
    uint32_t capabilities = 0;
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport = ZOO_SMB_TRANSPORT_TYPE_MIN;

    ZOO_BOOL parsed = zoo_smb_protocol_parse_discovery_payload_message_ex2(
        &msg,
        service,
        address,
        &port,
        topic,
        &transport,
        multicast,
        &multicast_port,
        host_fingerprint,
        &capabilities,
        shm_endpoint);

    TEST_ASSERT_FALSE(parsed);
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_make_and_parse_discovery_payload);
    RUN_TEST(test_header_validation);
    RUN_TEST(test_invalid_header_magic);
    RUN_TEST(test_invalid_header_version);
    RUN_TEST(test_protocol_constants);
    RUN_TEST(test_transport_type_validation);
    RUN_TEST(test_discovery_payload_ex2_round_trip_with_metadata);
    RUN_TEST(test_discovery_payload_ex2_parses_legacy_token_order);
    RUN_TEST(test_discovery_payload_ex2_rejects_unsafe_metadata_values);
    
    return UNITY_END();
}
