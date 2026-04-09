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
#define DEFAULT_PAYLOAD "hello from shm client"

#define DEMO_POOL_SIZE (2 * 1024 * 1024)
#define DEMO_THREAD_COUNT 2
#define DEMO_QUEUE_SIZE 128

typedef struct
{
    const char* channel;
    const char* server_name;
    const char* client_name;
    const char* topic;
    const char* payload;
    int timeout_ms;
    int log_level;
} DEMO_OPTIONS;

typedef enum
{
    PARSE_OK = 0,
    PARSE_HELP = 1,
    PARSE_ERROR = -1
} PARSE_RESULT;

static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_reply_received = 0;

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
    printf("  --server-name <name>   Receiver identity (default: %s)\n", DEFAULT_SERVER_NAME);
    printf("  --client-name <name>   Sender identity (default: shm_demo_client_<pid>)\n");
    printf("  --topic <name>         Message topic (default: %s)\n", DEFAULT_TOPIC);
    printf("  --payload <text>       Request payload (default: %s)\n", DEFAULT_PAYLOAD);
    printf("  --timeout-ms <ms>      Reply timeout in ms (default: 5000)\n");
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
        if ((strcmp(argv[i], "--client-name") == 0) && i + 1 < argc)
        {
            options->client_name = argv[++i];
            continue;
        }
        if ((strcmp(argv[i], "--topic") == 0) && i + 1 < argc)
        {
            options->topic = argv[++i];
            continue;
        }
        if ((strcmp(argv[i], "--payload") == 0) && i + 1 < argc)
        {
            options->payload = argv[++i];
            continue;
        }
        if ((strcmp(argv[i], "--timeout-ms") == 0) && i + 1 < argc)
        {
            options->timeout_ms = atoi(argv[++i]);
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

static void on_client_message(void* user_data, const ZOO_SMB_MSG_STRUCT* msg)
{
    (void)user_data;
    if (!msg)
    {
        return;
    }

    printf("Client received msg_type=%u from '%s'\n", msg->header.msg_type, msg->header.sender);
    fflush(stdout);

    if (msg->header.msg_type == ZOO_SMB_MSG_TYPE_REPL)
    {
        const char* payload = msg->payload ? (const char*)msg->payload : "<empty>";
        printf("Received reply from '%s': %s\n", msg->header.sender, payload);
        g_reply_received = 1;
        g_running = 0;
    }
}

static void fill_default_client_name(char* out, size_t out_size)
{
    snprintf(out, out_size, "shm_demo_client_%d", getpid());
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

int main(int argc, char** argv)
{
    char auto_client_name[32];
    fill_default_client_name(auto_client_name, sizeof(auto_client_name));

    DEMO_OPTIONS options = {
        .channel = DEFAULT_CHANNEL,
        .server_name = DEFAULT_SERVER_NAME,
        .client_name = auto_client_name,
        .topic = DEFAULT_TOPIC,
        .payload = DEFAULT_PAYLOAD,
        .timeout_ms = 5000,
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
    config.is_server = ZOO_FALSE;
    config.timeout_ms = (uint32_t)options.timeout_ms;
    snprintf(config.name, sizeof(config.name), "%s", options.channel);
    snprintf(config.address, sizeof(config.address), "%s", options.channel);

    ZOO_SMB_TRANSPORT_HANDLE transport = zoo_smb_create_transport(&config);
    if (!transport)
    {
        fprintf(stderr, "Failed to create SHM client transport\n");
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    TRANSPORT_DATA_OBSERVER_HANDLE observer = create_transport_data_observer(on_client_message, NULL);
    if (!observer || zoo_smb_transport_add_data_observer(transport, observer) != ZOO_SMB_OK)
    {
        fprintf(stderr, "Failed to register client observer\n");
        if (observer)
        {
            destroy_transport_data_observer(observer);
        }
        zoo_smb_destroy_transport(transport);
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    if (zoo_smb_transport_start(transport, ZOO_TRUE) != ZOO_SMB_OK)
    {
        fprintf(stderr, "Failed to start SHM client transport\n");
        zoo_smb_transport_remove_data_observer(transport, observer);
        destroy_transport_data_observer(observer);
        zoo_smb_destroy_transport(transport);
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    if (wait_for_transport_start(transport, 10000) != 0)
    {
        fprintf(stderr, "SHM client transport did not become ready\n");
        zoo_smb_transport_remove_data_observer(transport, observer);
        destroy_transport_data_observer(observer);
        zoo_smb_transport_stop(transport);
        zoo_smb_destroy_transport(transport);
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    ZOO_SMB_MSG_STRUCT req;
    memset(&req, 0, sizeof(req));
    req.header.magic = ZOO_SMB_MSG_MAGIC_NUMBER;
    req.header.version = ZOO_SMB_MSG_VERSION;
    req.header.msg_type = ZOO_SMB_MSG_TYPE_REQ;
    req.header.msg_id = 1;
    req.header.request_id = (uint64_t)time(NULL);
    req.header.timestamp = (uint64_t)time(NULL);
    req.header.payload_size = (uint32_t)(strlen(options.payload) + 1);
    snprintf(req.header.sender, sizeof(req.header.sender), "%s", options.client_name);
    snprintf(req.header.topic, sizeof(req.header.topic), "%s", options.topic);
    req.payload = (uint8_t*)options.payload;

    if (zoo_smb_transport_send(transport, &req, options.server_name) != ZOO_SMB_OK)
    {
        fprintf(stderr, "Failed to send request to '%s'\n", options.server_name);
        zoo_smb_transport_remove_data_observer(transport, observer);
        destroy_transport_data_observer(observer);
        zoo_smb_transport_stop(transport);
        zoo_smb_destroy_transport(transport);
        zoo_destroy_thread_pool(ZOO_FALSE);
        zoo_destroy_memory_pool();
        return EXIT_FAILURE;
    }

    printf("Request sent to '%s' via channel '%s'. Waiting for reply...\n", options.server_name, options.channel);

    int waited_ms = 0;
    while (g_running && waited_ms < options.timeout_ms)
    {
        usleep(10000);
        waited_ms += 10;
    }

    if (!g_reply_received)
    {
        fprintf(stderr, "No reply received within %d ms.\n", options.timeout_ms);
    }

    zoo_smb_transport_remove_data_observer(transport, observer);
    destroy_transport_data_observer(observer);
    zoo_smb_transport_stop(transport);
    zoo_smb_destroy_transport(transport);

    zoo_destroy_thread_pool(ZOO_FALSE);
    zoo_destroy_memory_pool();

    return g_reply_received ? EXIT_SUCCESS : EXIT_FAILURE;
}
