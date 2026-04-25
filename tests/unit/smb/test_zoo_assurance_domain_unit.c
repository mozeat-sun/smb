/*******************************************************************************
 * Copyright (C) 2026, ZOO Ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: ZOO_ASSURANCE_DOMAIN_UNIT
 * File name: test_zoo_assurance_domain_unit.c
 * Description: Unit tests for assurance mesh and domain profile foundation
 * Traceability coverage:
 * - REQ-SAFE-003: requirement-linked evidence for assurance startup policy.
 * - REQ-REL-001: profile-specific runtime startup compatibility validation.
 * - REQ-COMP-001: protocol major/minor compatibility fail-fast behavior.
 ******************************************************************************/

#include "unity.h"
#include <string.h>
#include "domain/zoo_domain_profile.h"
#include "domain/zoo_domain_policy.h"
#include "assurance/zoo_assurance_mesh.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_domain_profile_is_valid(void)
{
    ZOO_DOMAIN_PROFILE_ENUM profile = zoo_domain_profile_get_active();
    const char* profile_name = zoo_domain_profile_to_string(profile);

    TEST_ASSERT_NOT_NULL(profile_name);
    TEST_ASSERT_NOT_EQUAL(0, profile_name[0]);
}

void test_assurance_protocol_compatible_for_equal_major_and_lower_minor(void)
{
    ZOO_ASSURANCE_PROTOCOL_CONTEXT_STRUCT protocol = {0};
    protocol.local_wire_major = 1U;
    protocol.local_wire_minor = 3U;
    protocol.peer_wire_major = 1U;
    protocol.peer_wire_minor = 2U;

    TEST_ASSERT_EQUAL(
        ZOO_ASSURANCE_PROTOCOL_COMPATIBLE,
        zoo_assurance_check_protocol_compatibility(&protocol));
}

void test_assurance_protocol_incompatible_for_higher_peer_minor(void)
{
    ZOO_ASSURANCE_PROTOCOL_CONTEXT_STRUCT protocol = {0};
    protocol.local_wire_major = 1U;
    protocol.local_wire_minor = 0U;
    protocol.peer_wire_major = 1U;
    protocol.peer_wire_minor = 1U;

    TEST_ASSERT_EQUAL(
        ZOO_ASSURANCE_PROTOCOL_INCOMPATIBLE,
        zoo_assurance_check_protocol_compatibility(&protocol));
}

void test_assurance_startup_rejects_incompatible_protocol_for_industrial(void)
{
    ZOO_ASSURANCE_STARTUP_CONTEXT_STRUCT startup = {0};
    startup.domain_profile = ZOO_DOMAIN_PROFILE_INDUSTRIAL;
    startup.protocol.local_wire_major = 1U;
    startup.protocol.local_wire_minor = 0U;
    startup.protocol.peer_wire_major = 1U;
    startup.protocol.peer_wire_minor = 5U;

    ZOO_ERROR_TYPE result = zoo_assurance_evaluate_startup(&startup);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_VERSION_MISMATCH, (uint32_t)result);
}

void test_assurance_startup_allows_generic_profile_without_mesh(void)
{
    ZOO_ASSURANCE_STARTUP_CONTEXT_STRUCT startup = {0};
    startup.domain_profile = ZOO_DOMAIN_PROFILE_GENERIC;
    startup.protocol.local_wire_major = 1U;
    startup.protocol.local_wire_minor = 0U;
    startup.protocol.peer_wire_major = 7U;
    startup.protocol.peer_wire_minor = 9U;

    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_assurance_evaluate_startup(&startup));
}

void test_domain_policy_startup_decision_by_profile(void)
{
    TEST_ASSERT_EQUAL(
        ZOO_DOMAIN_STARTUP_DECISION_ALLOW,
        zoo_domain_policy_get_startup_decision(ZOO_DOMAIN_PROFILE_GENERIC));

    TEST_ASSERT_EQUAL(
        ZOO_DOMAIN_STARTUP_DECISION_REQUIRE_PROTOCOL_COMPATIBILITY,
        zoo_domain_policy_get_startup_decision(ZOO_DOMAIN_PROFILE_INDUSTRIAL));
}

void test_domain_policy_partition_ids_are_stable(void)
{
    TEST_ASSERT_EQUAL(0U, zoo_domain_policy_get_partition_id(ZOO_DOMAIN_PROFILE_GENERIC));
    TEST_ASSERT_EQUAL(10U, zoo_domain_policy_get_partition_id(ZOO_DOMAIN_PROFILE_INDUSTRIAL));
    TEST_ASSERT_EQUAL(20U, zoo_domain_policy_get_partition_id(ZOO_DOMAIN_PROFILE_AUTOMOTIVE));
    TEST_ASSERT_EQUAL(30U, zoo_domain_policy_get_partition_id(ZOO_DOMAIN_PROFILE_MILITARY));
}

void test_assurance_policy_snapshot_resolution(void)
{
    ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT snapshot;
    memset(&snapshot, 0, sizeof(snapshot));

    TEST_ASSERT_EQUAL(
        ZOO_SMB_OK,
        zoo_assurance_resolve_policy(ZOO_DOMAIN_PROFILE_AUTOMOTIVE, &snapshot));

    TEST_ASSERT_EQUAL(
        ZOO_DOMAIN_STARTUP_DECISION_REQUIRE_PROTOCOL_COMPATIBILITY,
        snapshot.startup_decision);
    TEST_ASSERT_EQUAL(ZOO_DOMAIN_ASSURANCE_CLASS_CONTROL, snapshot.assurance_class);
    TEST_ASSERT_EQUAL(20U, snapshot.partition_id);
}

void test_assurance_policy_snapshot_validation(void)
{
    ZOO_ASSURANCE_POLICY_SNAPSHOT_STRUCT snapshot;
    memset(&snapshot, 0, sizeof(snapshot));

    snapshot.startup_decision = ZOO_DOMAIN_STARTUP_DECISION_REQUIRE_PROTOCOL_COMPATIBILITY;
    snapshot.assurance_class = ZOO_DOMAIN_ASSURANCE_CLASS_CONTROL;
    snapshot.partition_id = 10U;

    TEST_ASSERT_EQUAL(
        ZOO_SMB_OK,
        zoo_assurance_validate_policy_snapshot(ZOO_DOMAIN_PROFILE_INDUSTRIAL, &snapshot));

    snapshot.partition_id = 0U;
    TEST_ASSERT_EQUAL_HEX32(
        (uint32_t)ZOO_SMB_ERROR_INVALID_STATE,
        (uint32_t)zoo_assurance_validate_policy_snapshot(ZOO_DOMAIN_PROFILE_INDUSTRIAL, &snapshot));
}

void test_domain_policy_identity_requirement(void)
{
    TEST_ASSERT_FALSE(zoo_domain_policy_requires_identity(ZOO_DOMAIN_PROFILE_GENERIC));
    TEST_ASSERT_TRUE(zoo_domain_policy_requires_identity(ZOO_DOMAIN_PROFILE_INDUSTRIAL));
    TEST_ASSERT_TRUE(zoo_domain_policy_requires_identity(ZOO_DOMAIN_PROFILE_AUTOMOTIVE));
    TEST_ASSERT_TRUE(zoo_domain_policy_requires_identity(ZOO_DOMAIN_PROFILE_MILITARY));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_domain_profile_is_valid);
    RUN_TEST(test_assurance_protocol_compatible_for_equal_major_and_lower_minor);
    RUN_TEST(test_assurance_protocol_incompatible_for_higher_peer_minor);
    RUN_TEST(test_assurance_startup_rejects_incompatible_protocol_for_industrial);
    RUN_TEST(test_assurance_startup_allows_generic_profile_without_mesh);
    RUN_TEST(test_domain_policy_startup_decision_by_profile);
    RUN_TEST(test_domain_policy_partition_ids_are_stable);
    RUN_TEST(test_assurance_policy_snapshot_resolution);
    RUN_TEST(test_assurance_policy_snapshot_validation);
    RUN_TEST(test_domain_policy_identity_requirement);

    return UNITY_END();
}
