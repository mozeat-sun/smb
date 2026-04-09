/*******************************************************************************
 * Copyright (C) 2026, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: UNIT_TEST
 * File name: test_zoo_dispatcher_smb.c
 * Description: Unit tests for generic dispatcher usage in SMB context
 ******************************************************************************/

#include "unity.h"
#include "zoo_dispatcher.h"
#include "zoo_memory_pool.h"
#include <pthread.h>

static ZOO_QUEUE_HANDLE g_queue;
static ZOO_DISPATCHER_HANDLE g_dispatcher;
static pthread_t g_dispatcher_thread;
static ZOO_BOOL g_dispatcher_thread_started;
static volatile int g_handler_calls;
static volatile int g_last_message_value;
static volatile int g_last_context_value;
static volatile int g_last_user_value;

static void dispatcher_test_handler(void* user_data, void* msg, void* context)
{
    int* user_value = (int*)user_data;
    int* msg_value = (int*)msg;
    int* ctx_value = (int*)context;

    g_handler_calls++;
    g_last_message_value = msg_value ? *msg_value : -1;
    g_last_context_value = ctx_value ? *ctx_value : -1;
    g_last_user_value = user_value ? *user_value : -1;

    if (msg_value)
    {
        zoo_free_to_pool(msg_value);
    }
    if (ctx_value)
    {
        zoo_free_to_pool(ctx_value);
    }
    if (user_value)
    {
        zoo_free_to_pool(user_value);
    }
}

static void* dispatcher_thread_main(void* arg)
{
    zoo_start_dispatcher((ZOO_DISPATCHER_HANDLE)arg);
    return NULL;
}

void setUp(void)
{
    zoo_create_memory_pool(2 * 1024 * 1024);
    g_queue = zoo_create_queue(8);
    g_dispatcher = zoo_create_dispatcher(g_queue);
    g_dispatcher_thread_started = ZOO_FALSE;
    g_handler_calls = 0;
    g_last_message_value = -1;
    g_last_context_value = -1;
    g_last_user_value = -1;
}

void tearDown(void)
{
    if (g_dispatcher_thread_started)
    {
        zoo_stop_dispatcher(g_dispatcher);
        (void)pthread_join(g_dispatcher_thread, NULL);
        g_dispatcher_thread_started = ZOO_FALSE;
    }

    if (g_dispatcher)
    {
        zoo_destroy_dispatcher(g_dispatcher);
        g_dispatcher = NULL;
    }

    if (g_queue)
    {
        zoo_destroy_queue(g_queue);
        g_queue = NULL;
    }

    zoo_destroy_memory_pool();
}

void test_CreateDispatcherWithQueue(void)
{
    TEST_ASSERT_NOT_NULL(g_queue);
    TEST_ASSERT_NOT_NULL(g_dispatcher);
}

void test_DispatcherExecutesQueuedHandler(void)
{
    int* msg_value = (int*)zoo_allocate_from_pool(sizeof(int));
    int* ctx_value = (int*)zoo_allocate_from_pool(sizeof(int));
    int* user_value = (int*)zoo_allocate_from_pool(sizeof(int));

    TEST_ASSERT_NOT_NULL(g_dispatcher);
    TEST_ASSERT_NOT_NULL(msg_value);
    TEST_ASSERT_NOT_NULL(ctx_value);
    TEST_ASSERT_NOT_NULL(user_value);

    *msg_value = 101;
    *ctx_value = 202;
    *user_value = 303;

    TEST_ASSERT_EQUAL_INT(0, pthread_create(&g_dispatcher_thread, NULL, dispatcher_thread_main, g_dispatcher));
    g_dispatcher_thread_started = ZOO_TRUE;

    TEST_ASSERT_TRUE(zoo_queue_enqueue(g_queue, msg_value, ctx_value, dispatcher_test_handler, user_value));

    for (int retry = 0; retry < 100 && g_handler_calls == 0; ++retry)
    {
        ZOO_SLEEP_MS(1);
    }

    TEST_ASSERT_EQUAL_INT(1, g_handler_calls);
    TEST_ASSERT_EQUAL_INT(101, g_last_message_value);
    TEST_ASSERT_EQUAL_INT(202, g_last_context_value);
    TEST_ASSERT_EQUAL_INT(303, g_last_user_value);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CreateDispatcherWithQueue);
    RUN_TEST(test_DispatcherExecutesQueuedHandler);

    return UNITY_END();
}