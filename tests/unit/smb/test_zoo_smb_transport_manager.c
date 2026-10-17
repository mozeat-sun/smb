#include "unity.h"
#include "test_smb_helpers.h"
#include "zoo_smb_transport_manager.h"
#include "zoo_smb_service_manager.h"
#include "zoo_smb_service.h"
#include "zoo_smb_transport.h"
#include "zoo_smb_config.h"
#include "zoo_smb_transport_shm_common.h"
#include <string.h>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

/*
 * Traceability coverage:
 * - REQ-REL-004: deterministic transport selection and fail-open fallback behavior.
 * - REQ-SAFE-003: requirement-linked unit evidence for resolver decision matrix.
 */

static ZOO_SMB_CONFIG_STRUCT g_cfg;

void setUp(void)
{
    test_memory_tracker_init();
    const ZOO_SMB_CONFIG_STRUCT* base = zoo_smb_config_init();
    TEST_ASSERT_NOT_NULL(base);
    memcpy(&g_cfg, base, sizeof(g_cfg));
    g_cfg.local_machine.address[sizeof(g_cfg.local_machine.address) - 1] = '\0';
    g_cfg.sys.transport_shm_probe_timeout_ms = 20;
    g_cfg.sys.transport_shm_fallback_cooldown_ms = 200;
}

void tearDown(void)
{
    test_memory_tracker_cleanup();
}

static ZOO_SMB_SERVICE_HANDLE make_registration_service(
    const char* name,
    ZOO_SMB_TRANSPORT_TYPE_ENUM transport_type)
{
    return zoo_smb_create_service(
        ZOO_SMB_SERVICE_TYPE_REGISTRATION,
        name,
        "matrix/topic",
        "127.0.0.1",
        19000,
        NULL,
        0,
        transport_type,
        1000);
}

void test_resolver_matrix_auto_select_off_keeps_network_transport(void)
{
    g_cfg.sys.transport_auto_select = 0;

    ZOO_SMB_SERVICE_MANAGER_HANDLE sm = zoo_smb_create_service_manager(&g_cfg);
    TEST_ASSERT_NOT_NULL(sm);
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE tm = zoo_smb_create_transport_manager(sm, &g_cfg);
    TEST_ASSERT_NOT_NULL(tm);

    ZOO_SMB_SERVICE_HANDLE svc = make_registration_service("matrix_auto_off", ZOO_SMB_TRANSPORT_TYPE_TCP);
    TEST_ASSERT_NOT_NULL(svc);
    snprintf(svc->address, sizeof(svc->address), "%s", g_cfg.local_machine.address);
    svc->address[sizeof(svc->address) - 1] = '\0';
    snprintf(svc->host_fingerprint, sizeof(svc->host_fingerprint), "%s", g_cfg.local_machine.address);
    svc->host_fingerprint[sizeof(svc->host_fingerprint) - 1] = '\0';
    svc->transport_capabilities = ZOO_SMB_SERVICE_CAPABILITY_SHM | ZOO_SMB_SERVICE_CAPABILITY_TCP;
    snprintf(svc->shm_endpoint, sizeof(svc->shm_endpoint), "%s", "matrix_auto_off");
    svc->shm_endpoint[sizeof(svc->shm_endpoint) - 1] = '\0';

    ZOO_SMB_TRANSPORT_HANDLE tp = zoo_smb_transport_manager_make_transport(tm, svc);
    TEST_ASSERT_NOT_NULL(tp);
    const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* tp_cfg = zoo_smb_transport_get_config(tp);
    TEST_ASSERT_NOT_NULL(tp_cfg);
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_TCP, tp_cfg->type);

    ZOO_SMB_TRANSPORT_AUTO_SELECT_METRICS_STRUCT metrics;
    memset(&metrics, 0, sizeof(metrics));
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_manager_get_auto_select_metrics(tm, &metrics));
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_TCP, metrics.last_selected_transport_type);
    TEST_ASSERT_TRUE(metrics.network_selected_total >= 1U);

    zoo_smb_destroy_transport_manager(tm);
    zoo_smb_destroy_service(svc);
    zoo_smb_destroy_service_manager(sm);
}

void test_resolver_matrix_same_host_shm_ready_prefers_shm(void)
{
#if !(defined(__unix__) || defined(__APPLE__))
    TEST_IGNORE_MESSAGE("SHM readiness probe requires SysV IPC support");
#endif

    g_cfg.sys.transport_auto_select = 1;

    ZOO_SMB_SERVICE_MANAGER_HANDLE sm = zoo_smb_create_service_manager(&g_cfg);
    TEST_ASSERT_NOT_NULL(sm);
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE tm = zoo_smb_create_transport_manager(sm, &g_cfg);
    TEST_ASSERT_NOT_NULL(tm);

    ZOO_SMB_SERVICE_HANDLE svc = make_registration_service("matrix_shm_ready", ZOO_SMB_TRANSPORT_TYPE_TCP);
    TEST_ASSERT_NOT_NULL(svc);
    snprintf(svc->address, sizeof(svc->address), "%s", g_cfg.local_machine.address);
    svc->address[sizeof(svc->address) - 1] = '\0';
    snprintf(svc->host_fingerprint, sizeof(svc->host_fingerprint), "%s", g_cfg.local_machine.address);
    svc->host_fingerprint[sizeof(svc->host_fingerprint) - 1] = '\0';
    svc->transport_capabilities = ZOO_SMB_SERVICE_CAPABILITY_SHM | ZOO_SMB_SERVICE_CAPABILITY_TCP;
    snprintf(svc->shm_endpoint, sizeof(svc->shm_endpoint), "%s", "matrix_shm_ready");
    svc->shm_endpoint[sizeof(svc->shm_endpoint) - 1] = '\0';

    int probe_shm_id = -1;
    key_t probe_key = shm_generate_key(svc->shm_endpoint, 0);
    if (probe_key != (key_t)-1)
    {
        probe_shm_id = shmget(probe_key, 256, IPC_CREAT | 0666);
    }

    ZOO_SMB_TRANSPORT_HANDLE tp = zoo_smb_transport_manager_make_transport(tm, svc);
    TEST_ASSERT_NOT_NULL(tp);
    const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* tp_cfg = zoo_smb_transport_get_config(tp);
    TEST_ASSERT_NOT_NULL(tp_cfg);
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_SHM, tp_cfg->type);

    ZOO_SMB_TRANSPORT_AUTO_SELECT_METRICS_STRUCT metrics;
    memset(&metrics, 0, sizeof(metrics));
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_manager_get_auto_select_metrics(tm, &metrics));
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_SHM, metrics.last_selected_transport_type);
    TEST_ASSERT_TRUE(metrics.shm_selected_total >= 1U);

    if (probe_shm_id >= 0)
    {
        (void)shmctl(probe_shm_id, IPC_RMID, NULL);
    }
    zoo_smb_destroy_transport_manager(tm);
    zoo_smb_destroy_service(svc);
    zoo_smb_destroy_service_manager(sm);
}

void test_resolver_matrix_same_host_shm_not_ready_falls_back_to_network(void)
{
    g_cfg.sys.transport_auto_select = 1;

    ZOO_SMB_SERVICE_MANAGER_HANDLE sm = zoo_smb_create_service_manager(&g_cfg);
    TEST_ASSERT_NOT_NULL(sm);
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE tm = zoo_smb_create_transport_manager(sm, &g_cfg);
    TEST_ASSERT_NOT_NULL(tm);

    ZOO_SMB_SERVICE_HANDLE svc = make_registration_service("matrix_shm_not_ready", ZOO_SMB_TRANSPORT_TYPE_TCP);
    TEST_ASSERT_NOT_NULL(svc);
    snprintf(svc->address, sizeof(svc->address), "%s", g_cfg.local_machine.address);
    svc->address[sizeof(svc->address) - 1] = '\0';
    snprintf(svc->host_fingerprint, sizeof(svc->host_fingerprint), "%s", g_cfg.local_machine.address);
    svc->host_fingerprint[sizeof(svc->host_fingerprint) - 1] = '\0';
    svc->transport_capabilities = ZOO_SMB_SERVICE_CAPABILITY_SHM | ZOO_SMB_SERVICE_CAPABILITY_TCP;
    snprintf(svc->shm_endpoint, sizeof(svc->shm_endpoint), "%s", "matrix_no_such_shm_endpoint");
    svc->shm_endpoint[sizeof(svc->shm_endpoint) - 1] = '\0';

    ZOO_SMB_TRANSPORT_HANDLE tp = zoo_smb_transport_manager_make_transport(tm, svc);
    TEST_ASSERT_NOT_NULL(tp);
    const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* tp_cfg = zoo_smb_transport_get_config(tp);
    TEST_ASSERT_NOT_NULL(tp_cfg);
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_TCP, tp_cfg->type);

    ZOO_SMB_TRANSPORT_AUTO_SELECT_METRICS_STRUCT metrics;
    memset(&metrics, 0, sizeof(metrics));
    TEST_ASSERT_EQUAL(ZOO_SMB_OK, zoo_smb_transport_manager_get_auto_select_metrics(tm, &metrics));
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_TCP, metrics.last_selected_transport_type);
    TEST_ASSERT_TRUE(metrics.fallback_to_network_total >= 1U);

    zoo_smb_destroy_transport_manager(tm);
    zoo_smb_destroy_service(svc);
    zoo_smb_destroy_service_manager(sm);
}

void test_resolver_matrix_explicit_shm_service_stays_shm(void)
{
    g_cfg.sys.transport_auto_select = 1;

    ZOO_SMB_SERVICE_MANAGER_HANDLE sm = zoo_smb_create_service_manager(&g_cfg);
    TEST_ASSERT_NOT_NULL(sm);
    ZOO_SMB_TRANSPORT_MANAGER_HANDLE tm = zoo_smb_create_transport_manager(sm, &g_cfg);
    TEST_ASSERT_NOT_NULL(tm);

    ZOO_SMB_SERVICE_HANDLE svc = make_registration_service("matrix_explicit_shm", ZOO_SMB_TRANSPORT_TYPE_SHM);
    TEST_ASSERT_NOT_NULL(svc);

    ZOO_SMB_TRANSPORT_HANDLE tp = zoo_smb_transport_manager_make_transport(tm, svc);
    TEST_ASSERT_NOT_NULL(tp);
    const ZOO_SMB_TRANSPORT_CONFIG_STRUCT* tp_cfg = zoo_smb_transport_get_config(tp);
    TEST_ASSERT_NOT_NULL(tp_cfg);
    TEST_ASSERT_EQUAL_INT(ZOO_SMB_TRANSPORT_TYPE_SHM, tp_cfg->type);

    zoo_smb_destroy_transport_manager(tm);
    zoo_smb_destroy_service(svc);
    zoo_smb_destroy_service_manager(sm);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_resolver_matrix_auto_select_off_keeps_network_transport);
    RUN_TEST(test_resolver_matrix_same_host_shm_ready_prefers_shm);
    RUN_TEST(test_resolver_matrix_same_host_shm_not_ready_falls_back_to_network);
    RUN_TEST(test_resolver_matrix_explicit_shm_service_stays_shm);

    return UNITY_END();
}

