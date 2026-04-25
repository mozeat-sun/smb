/*******************************************************************************
 * Copyright (C) 2026, ZOO Ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: ZOO_SMB_RUNTIME_ASSURANCE_UNIT
 * File name: test_zoo_smb_runtime_assurance_unit.c
 * Description: Unit tests for runtime assurance startup composition behavior
 * Traceability coverage:
 * - REQ-REL-001: profile-aware runtime startup policy checks.
 * - REQ-COMP-001: protocol compatibility fail-fast behavior at startup.
 ******************************************************************************/

#include <string.h>
#include "unity.h"
#include "zoo_smb.h"
#include "zoo_smb_runtime.h"
#include "domain/zoo_domain_profile.h"

static ZOO_SMB_RUNTIME_HANDLE g_runtime = NULL;

void setUp(void)
{
    g_runtime = NULL;
}

void tearDown(void)
{
    if (g_runtime != NULL)
    {
        zoo_smb_runtime_destroy(g_runtime);
        g_runtime = NULL;
    }
}

void test_runtime_startup_with_compatible_protocol_succeeds(void)
{
    if (!zoo_smb_is_ready())
    {
        TEST_IGNORE_MESSAGE("SMB runtime is not ready in this environment");
    }

    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_init();
    TEST_ASSERT_NOT_NULL(config);

    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.local_wire_major = 1U;
    options.local_wire_minor = 3U;
    options.peer_wire_major = 1U;
    options.peer_wire_minor = 2U;

    g_runtime = zoo_smb_runtime_create_ex(config, &options);
    TEST_ASSERT_NOT_NULL(g_runtime);

    ZOO_ERROR_TYPE result = zoo_smb_runtime_start(g_runtime);
    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK, (uint32_t)result);
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_STATE_RUNNING, zoo_smb_runtime_get_state(g_runtime));
}

void test_runtime_startup_incompatible_protocol_is_profile_gated(void)
{
    if (!zoo_smb_is_ready())
    {
        TEST_IGNORE_MESSAGE("SMB runtime is not ready in this environment");
    }

    const ZOO_SMB_CONFIG_STRUCT* config = zoo_smb_config_init();
    TEST_ASSERT_NOT_NULL(config);

    ZOO_SMB_RUNTIME_OPTIONS_STRUCT options;
    memset(&options, 0, sizeof(options));
    options.local_wire_major = 1U;
    options.local_wire_minor = 0U;
    options.peer_wire_major = 1U;
    options.peer_wire_minor = 9U;

    g_runtime = zoo_smb_runtime_create_ex(config, &options);
    TEST_ASSERT_NOT_NULL(g_runtime);

    ZOO_ERROR_TYPE result = zoo_smb_runtime_start(g_runtime);
    if (zoo_domain_profile_uses_assurance_mesh(zoo_domain_profile_get_active()))
    {
        TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_ERROR_VERSION_MISMATCH, (uint32_t)result);
        TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_STATE_FAULTED, zoo_smb_runtime_get_state(g_runtime));
        return;
    }

    TEST_ASSERT_EQUAL_HEX32((uint32_t)ZOO_SMB_OK, (uint32_t)result);
    TEST_ASSERT_EQUAL(ZOO_SMB_RUNTIME_STATE_RUNNING, zoo_smb_runtime_get_state(g_runtime));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_runtime_startup_with_compatible_protocol_succeeds);
    RUN_TEST(test_runtime_startup_incompatible_protocol_is_profile_gated);

    return UNITY_END();
}
