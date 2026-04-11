/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus Tests
 * Component id: test_error_
 * File name: test_error.c
 * Description: Unity-based error handling tests (converted from GTest)
 * Traceability coverage:
 * - REQ-SAFE-003: requirement-linked unit evidence for SMB error reporting primitives.
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-08-05     AI                converted from GTest
 ******************************************************************************/

#include "unity.h"
#include "utility/zoo_smb_error.h"
#include <string.h>

static char last_dump_info[1024];

void custom_dump_handler(const char *info)
{
    strncpy(last_dump_info, info ? info : "", sizeof(last_dump_info) - 1);
    last_dump_info[sizeof(last_dump_info) - 1] = '\0';
}

void test_error_string_not_null(void)
{
    TEST_ASSERT_NOT_NULL(zoo_smb_get_error_string(ZOO_SMB_OK));
    TEST_ASSERT_NOT_NULL(zoo_smb_get_error_string(ZOO_SMB_ERROR_ALLOCATION_FAILED));
}

void test_set_and_get_last_error(void)
{
    zoo_smb_set_last_error(ZOO_SMB_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQUAL_UINT32(ZOO_SMB_ERROR_INVALID_PARAM, zoo_smb_get_last_error());

    zoo_smb_set_last_error(ZOO_SMB_ERROR_TIMEOUT);
    TEST_ASSERT_EQUAL_UINT32(ZOO_SMB_ERROR_TIMEOUT, zoo_smb_get_last_error());
}

void test_all_error_codes_have_string(void)
{
    for (unsigned int code = ZOO_SMB_OK; code >= ZOO_SMB_ERROR_NOT_STARTED; --code)
    {
        const char *desc = zoo_smb_get_error_string((ZOO_ERROR_TYPE)code);
        char msg[256];
        snprintf(msg, sizeof(msg), "Error code %u should have description", code);
        TEST_ASSERT_NOT_NULL_MESSAGE(desc, msg);
    }
}

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_error_string_not_null);
    RUN_TEST(test_set_and_get_last_error);
    RUN_TEST(test_all_error_codes_have_string);
    
    return UNITY_END();
}
