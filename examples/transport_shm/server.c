#include "zoo_smb_transport.h"
#include "zoo_smb_transport_observer.h"
#include "zoo_smb_message.h"
#include "zoo_memory_pool.h"
#include "zoo_thread_pool.h"
#include "zoo_log.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_CHANNEL "zoo_shm_demo"
#define DEFAULT_SERVER_NAME "shm_demo_server"
#define DEFAULT_TOPIC "demo/shm"

#define DEMO_POOL_SIZE (2 * 1024 * 1024)
#define DEMO_THREAD_COUNT 2
#define DEMO_QUEUE_SIZE 128

typedef struct
{
    const char* channel;
    const char* server_name;
    const char* topic;
    int log_level;
} DEMO_OPTIONS;

typedef enum
{
    PARSE_OK = 0,
    PARSE_HELP = 1,
    PARSE_ERROR = -1
} PARSE_RESULT;

static volatile sig_atomic_t g_running = 1;
static ZOO_SMB_TRANSPORT_HANDLE g_transport = NULL;
static char g_server_sender[32] = DEFAULT_SERVER_NAME;
static char g_expected_topic[32] = DEFAULT_TOPIC;

static void on_signal(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        g_running = 0;
    }
}

static int setup_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) != 0 || sigaction(SIGTERM, &sa, NULL) != 0)
    {
        perror("sigaction");
        return -1;
    }
    return 0;
}

static void print_usage(const char* program)
{
    printf("Usage: %s [options]\n", program);
    printf("Options:\n");
    printf("  --channel <name>       SHM channel name (default: %s)\n", DEFAULT_CHANNEL);
    printf("  --server-name <name>   Logical server identity (default: %s)\n", DEFAULT_SERVER_NAME);
    printf("  --topic <name>         Message topic (default: %s)\n", DEFAULT_TOPIC);
    printf("  --log-level <0-6>      TRACE..OFF (default: 2)\n");
    printf("  -h, --help             Show this help message\n");
}

static PARSE_RESULT parse_args(int argc, char** argv, DEMO_OPTIONS* options)
{
    for (int i = 1; i < argc; ++i)
    {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
        {
            print_usage(argv[0]);
            return PARSE_HELP;
        }
        if ((strcmp(argv[i], "--channel") == 0) && i + 1 < argc)
        {
            options->channel = argv[++i];
            continue;
        }
        if ((strcmp(argv[i], "--server-name") == 0) && i + 1 < argc)
        {
            options->server_name = argv[++i];
            continue;
        }
        if ((strcmp(argv[i], "--topic") == 0) && i + 1 < argc)
        {
            options->topic = argv[++i];
            continue;
        }
        if ((strcmp(argv[i], "--log-level") == 0) && i + 1 < argc)
        {
            options->log_level = atoi(argv[++i]);
            continue;
        }

        fprintf(stderr, "Unknown argument: %s\n", argv[i]);
        print_usage(argv[0]);
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static int wait_for_transport_start(ZOO_SMB_TRANSPORT_HANDLE transport, int timeout_ms)
{
    int waited_ms = 0;
    while (waited_ms < timeout_ms)
    {
        if (zoo_smb_transport_is_started(transport))
        {
            return 0;
        }
        usleep(50000);
        waited_ms += 50;
    }
    return -1;
}

static void send_reply_to_client(const ZOO_SMB_MSG_STRUCT* req)
{
    char reply_payload[256];
    int written = snprintf(reply_payload,
                           sizeof(reply_payload),
                           "SHM server received message %u (%u bytes)",
                           req->header.msg_id,
                           req->header.payload_size);
    if (written < 0)
    {
        return;
    }

    ZOO_SMB_MSG_STRUCT reply;
    memset(&reply, 0, sizeof(reply));
    reply.header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    reply.header.version = ZOO_SMB_MSG_VERSION;
    reply.header.msg_type = ZOO_SMB_MSG_TYPE_REPL;
    reply.header.msg_id = req->header.msg_id;
    reply.header.request_id = req->header.request_id;
    reply.header.payload_size = (uint32_t)(strlen(reply_payload) + 1);
    reply.header.timestamp = (uint64_t)time(NULL);
    snprintf(reply.header.sender, sizeof(reply.header.sender), "%s", g_server_sender);
    snprintf(reply.header.topic, sizeof(reply.header.topic), "%s", req->header.topic);
    reply.payload = (uint8_t*)reply_payload;

    if (zoo_smb_transport_send(g_transport, &reply, req->header.sender) != ZOO_SMB_OK)
    {
        fprintf(stderr, "Failed to send reply to %s\n", req->header.sender);
    }
    else
    {
        printf("Reply sent to '%s'\n", req->header.sender);
        fflush(stdout);
    }
}

static void on_server_message(void* user_data, const ZOO_SMB_MSG_STRUCT* msg)
{
    (void)user_data;
    if (!msg)
    {
        return;
    }

    if (strncmp(msg->header.topic, g_expected_topic, sizeof(msg->header.topic)) != 0)
    {
        printf("Ignored message for topic '%s' (expected '%s')\n", msg->header.topic, g_expected_topic);
        fflush(stdout);
        return;
    }

    const char* payload = msg->payload ? (const char*)msg->payload : "<empty>";
    printf("Received msg_type=%u msg_id=%u request_id=%llu sender=%s payload='%s'\n",
           msg->header.msg_type,
           msg->header.msg_id,
           (unsigned long long)msg->header.request_id,
           msg->header.sender,
           payload);
    fflush(stdout);

    send_reply_to_client(msg);
}

int main(int argc, char** argv)
{
    DEMO_OPTIONS options = {
        .channel = DEFAULT_CHANNEL,
        .server_name = DEFAULT_SERVER_NAME,
        .topic = DEFAULT_TOPIC,
        .log_level = ZOO_LOG_LEVEL_INFO,
    };

    PARSE_RESULT parse_result = parse_args(argc, argv, &options);
    if (parse_result == PARSE_HELP)
    {
        return EXIT_SUCCESS;
    }
    if (parse_result != PARSE_OK)
    {
        return EXIT_FAILURE;
    }

    zoo_log_set_level((ZOO_LOG_LEVEL_ENUM)options.log_level);

    if (zoo_create_memory_pool(DEMO_POOL_SIZE) != ZOO_OK)
    {
        fprintf(stderr, "Failed to initialize memory pool\n");
        return EXIT_FAILURE;
    }
    if (!zoo_create_thread_pool(DEMO_THREAD_COUNT, DEMO_QUEUE_SIZE))
    {
        fprintf(stderr, "Failed to initialize thread pool\n");
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    if (setup_signals() != 0)
    {
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    ZOO_SMB_TRANSPORT_CONFIG_STRUCT config;
    memset(&config, 0, sizeof(config));
    config.type = ZOO_SMB_TRANSPORT_TYPE_SHM;
    config.is_server = ZOO_TRUE;
    config.timeout_ms = 3000;
    snprintf(config.name, sizeof(config.name), "%s", options.channel);
    snprintf(config.address, sizeof(config.address), "%s", options.channel);
    snprintf(g_server_sender, sizeof(g_server_sender), "%s", options.server_name);
    snprintf(g_expected_topic, sizeof(g_expected_topic), "%s", options.topic);

    g_transport = zoo_smb_create_transport(&config);
    if (!g_transport)
    {
        fprintf(stderr, "Failed to create SHM server transport\n");
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    TRANSPORT_DATA_OBSERVER_HANDLE observer = create_transport_data_observer(on_server_message, NULL);
    if (!observer || zoo_smb_transport_add_data_observer(g_transport, observer) != ZOO_SMB_OK)
    {
        fprintf(stderr, "Failed to register server observer\n");
        if (observer)
        {
            destroy_transport_data_observer(observer);
        }
        zoo_smb_destroy_transport(g_transport);
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    if (zoo_smb_transport_start(g_transport, ZOO_TRUE) != ZOO_SMB_OK)
    {
        fprintf(stderr, "Failed to start SHM server transport\n");
        zoo_smb_transport_remove_data_observer(g_transport, observer);
        destroy_transport_data_observer(observer);
        zoo_smb_destroy_transport(g_transport);
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    if (wait_for_transport_start(g_transport, 10000) != 0)
    {
        fprintf(stderr, "SHM server transport did not become ready\n");
        zoo_smb_transport_remove_data_observer(g_transport, observer);
        destroy_transport_data_observer(observer);
        zoo_smb_transport_stop(g_transport);
        zoo_smb_destroy_transport(g_transport);
        g_transport = NULL;
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    printf("SHM server ready: channel='%s', server='%s'\n", options.channel, options.server_name);
    printf("Press Ctrl+C to stop.\n");

    while (g_running)
    {
        usleep(100000);
    }

    zoo_smb_transport_remove_data_observer(g_transport, observer);
    destroy_transport_data_observer(observer);
    zoo_smb_transport_stop(g_transport);
    zoo_smb_destroy_transport(g_transport);
    g_transport = NULL;

    zoo_destroy_thread_pool(ZOO_FALSE);
    zoo_destroy_memory_pool();

    return EXIT_SUCCESS;
}
