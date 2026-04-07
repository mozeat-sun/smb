/*******************************************************************************
 * Copyright (C) 2025, Basic Software Research Institute ltd
 * All rights reserved.
 * Product: ZOO
 * Module: Soft Message Bus
 * Component id: ZOO_SMB_CONFIG
 * File name: zoo_smb_config.c
 * Description: Configuration management implementation for ZOO SMB using libconfig
 * History recorder:
 * Version   date           author            context
 * 1.0       2025-05-14     weiwang.sun      created
 * 1.1       2025-06-19     weiwang.sun   enhanced initialization with file creation
 * 2.0       2025-07-25     ai.assistant  migrated to libconfig library
 ******************************************************************************/

#include "zoo_smb_config.h"
#include "zoo_log.h"
#include "zoo_smb_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#if __has_include(<libconfig.h>)
#include <libconfig.h>
#else
#include "libconfig_stub.h"
#endif

/* Default configuration file path */
#ifndef SMB_CONFIG_FILE_PATH
#define SMB_CONFIG_FILE_PATH "/etc/zoo/smb.conf"
#endif

/* Default configuration values */
static const ZOO_SMB_CONFIG_STRUCT ZOO_SMB_CONFIG_DEFAULT = {
    .local_machine = {
        .address = "127.0.0.1",
        .port_max = 15800,
        .port_min = 13800
    },
    .broadcast = {
        .address = "255.255.255.255", 
        .port = 50001, 
        .interval_ms = 1000, 
        .max_check_timeout_ms = 6000
    },
    .multicast = {
        .address = "224.1.1.100", 
        .port = 49000, 
        .ttl = 1, 
        .interface = "0.0.0.0", 
        .loopback = ZOO_TRUE
    },
    .log = {
        .file_path = "/var/log/zoo_smb.log", 
        .level = ZOO_LOG_LEVEL_INFO, 
        .target = ZOO_LOG_TARGET_CONSOLE, 
        .max_file_size = 1048576, 
        .max_backup_files = 5
    },
    .bridge = {
        .enable_bridge = ZOO_FALSE, 
        .from_service = "", 
        .to_service = "", 
        .from_topic = "", 
        .to_topic = "", 
        .from_transport_type = 0, 
        .to_transport_type = 0
    },
    .sys = {
        .default_thread_numbers = 4, 
        .max_thread_queue_size = 128, 
        .send_high_watermark = 96,
        .send_low_watermark = 64,
        .ingress_high_watermark = 768,
        .ingress_low_watermark = 512,
        .enable_transport_telemetry = ZOO_TRUE,
        .enable_routing_telemetry = ZOO_TRUE,
        .enforce_encrypted_messages = ZOO_FALSE,
        .require_sender_identity = ZOO_TRUE,
        .reuse_transport = ZOO_TRUE, 
        .mem_pool_size = 4 * 1024 * 1024, 
        .max_queue_size = 10240, 
        .max_node_size = 128, 
        .max_timeout = 5000, 
        .max_transport_buffer_size = 40 * 1024, 
        .msg_flag = 0, 
        .default_transport_type = ZOO_SMB_TRANSPORT_TYPE_SHM, 
        .transport_auto_select = 0
    }
};

/* Global configuration instance */
static ZOO_SMB_CONFIG_STRUCT g_smb_config;
static ZOO_BOOL g_config_initialized = ZOO_FALSE;

/* Function prototypes */
static ZOO_BOOL zoo_smb_config_load_from_file(ZOO_SMB_CONFIG_STRUCT* config, const char* file_path);
static ZOO_BOOL create_default_config_file(const char* file_path);

/**
 * @brief Check if a file exists
 * @param file_path Path to the file to check
 * @return ZOO_TRUE if file exists and is accessible, ZOO_FALSE otherwise
 * @note Uses access() system call with F_OK flag for existence check
 */
static ZOO_BOOL file_exists(const char* file_path)
{
    return file_path && (access(file_path, F_OK) == 0);
}

/**
 * @brief Create directory structure for a given file path
 * @param file_path Full path to a file, directories will be created for its parent path
 * @return ZOO_TRUE if directories exist or were created successfully, ZOO_FALSE on error
 * @details Recursively creates all parent directories with permissions 0755
 * @note Does not create the file itself, only the directory structure
 */
static ZOO_BOOL create_directories(const char* file_path)
{
    if (!file_path)
        return ZOO_FALSE;

    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s", file_path);

    char* last_slash = strrchr(dir_path, '/');
    if (!last_slash)
        return ZOO_TRUE;

    *last_slash = '\0';

    struct stat st;
    if (stat(dir_path, &st) == 0)
    {
        return S_ISDIR(st.st_mode);
    }

    // Create parent directories recursively
    char temp_path[512];
    char* pos = dir_path;
    while ((pos = strchr(pos + 1, '/')) != NULL)
    {
        size_t len = pos - dir_path;
        strncpy(temp_path, dir_path, len);
        temp_path[len] = '\0';
        if (mkdir(temp_path, 0755) != 0 && errno != EEXIST)
        {
            return ZOO_FALSE;
        }
    }

    return (mkdir(dir_path, 0755) == 0 || errno == EEXIST);
}

/**
 * @brief Safely retrieve string value from configuration setting
 * @param setting libconfig setting to query
 * @param name Name of the string parameter
 * @param default_value Default value to return if parameter not found
 * @return Configuration value if found, default_value otherwise
 * @note Always returns a valid string pointer
 */
static const char* get_config_string(config_setting_t* setting, const char* name, const char* default_value)
{
    const char* value;
    return (config_setting_lookup_string(setting, name, &value) == CONFIG_TRUE) ? value : default_value;
}

/**
 * @brief Safely retrieve integer value from configuration setting
 * @param setting libconfig setting to query
 * @param name Name of the integer parameter
 * @param default_value Default value to return if parameter not found
 * @return Configuration value if found, default_value otherwise
 */
static int get_config_int(config_setting_t* setting, const char* name, int default_value)
{
    int value;
    return (config_setting_lookup_int(setting, name, &value) == CONFIG_TRUE) ? value : default_value;
}

/**
 * @brief Safely retrieve boolean value from configuration setting
 * @param setting libconfig setting to query
 * @param name Name of the boolean parameter
 * @param default_value Default value to return if parameter not found
 * @return Configuration value if found, default_value otherwise
 */
static ZOO_BOOL get_config_bool(config_setting_t* setting, const char* name, ZOO_BOOL default_value)
{
    int value;
    return (config_setting_lookup_bool(setting, name, &value) == CONFIG_TRUE) ? (value != 0) : default_value;
}

/**
 * @brief Copy string with guaranteed null termination
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 * @note Ensures the destination string is always null-terminated
 */
static void safe_string_copy(char* dest, const char* src, size_t dest_size)
{
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

/**
 * @brief Load local machine configuration section
 * @param cfg libconfig configuration object
 * @param config Configuration structure to populate
 * @details Loads address, port_min, and port_max settings for local machine
 */
static void load_local_machine_config(config_t* cfg, ZOO_SMB_CONFIG_STRUCT* config)
{
    config_setting_t* section = config_lookup(cfg, "local_machine");
    if (!section)
        return;

    const char* addr = get_config_string(section, "address", config->local_machine.address);
    safe_string_copy(config->local_machine.address, addr, sizeof(config->local_machine.address));

    config->local_machine.port_min = get_config_int(section, "port_min", config->local_machine.port_min);
    config->local_machine.port_max = get_config_int(section, "port_max", config->local_machine.port_max);

    printf("[DEBUG] Local machine: %s:%d-%d\n",
           config->local_machine.address,
           config->local_machine.port_min,
           config->local_machine.port_max);
}

/**
 * @brief Load broadcast configuration section
 * @param cfg libconfig configuration object
 * @param config Configuration structure to populate
 * @details Loads broadcast address, port, interval, and timeout settings
 */
static void load_broadcast_config(config_t* cfg, ZOO_SMB_CONFIG_STRUCT* config)
{
    config_setting_t* section = config_lookup(cfg, "broadcast");
    if (!section)
        return;

    const char* addr = get_config_string(section, "address", config->broadcast.address);
    safe_string_copy(config->broadcast.address, addr, sizeof(config->broadcast.address));

    config->broadcast.port = get_config_int(section, "port", config->broadcast.port);
    config->broadcast.interval_ms = get_config_int(section, "interval_ms", config->broadcast.interval_ms);
    config->broadcast.max_check_timeout_ms = get_config_int(section, "max_check_timeout_ms", config->broadcast.max_check_timeout_ms);

    printf("[DEBUG] Broadcast: %s:%d\n", config->broadcast.address, config->broadcast.port);
}

/**
 * @brief Load multicast configuration section
 * @param cfg libconfig configuration object
 * @param config Configuration structure to populate
 * @details Loads multicast address, port, TTL, interface, and loopback settings
 */
static void load_multicast_config(config_t* cfg, ZOO_SMB_CONFIG_STRUCT* config)
{
    config_setting_t* section = config_lookup(cfg, "multicast");
    if (!section)
        return;

    const char* addr = get_config_string(section, "address", config->multicast.address);
    safe_string_copy(config->multicast.address, addr, sizeof(config->multicast.address));

    const char* interface = get_config_string(section, "interface", config->multicast.interface);
    safe_string_copy(config->multicast.interface, interface, sizeof(config->multicast.interface));

    config->multicast.port = get_config_int(section, "port", config->multicast.port);
    config->multicast.ttl = get_config_int(section, "ttl", config->multicast.ttl);
    config->multicast.loopback = get_config_bool(section, "loopback", config->multicast.loopback);

    printf("[DEBUG] Multicast: %s:%d (TTL:%d)\n",
           config->multicast.address,
           config->multicast.port,
           config->multicast.ttl);
}

/**
 * @brief Load system configuration section
 * @param cfg libconfig configuration object
 * @param config Configuration structure to populate
 * @details Loads all system-level settings including threads, memory, timeouts, etc.
 */
static void load_sys_config(config_t* cfg, ZOO_SMB_CONFIG_STRUCT* config)
{
    config_setting_t* section = config_lookup(cfg, "sys");
    if (!section)
        return;

    config->sys.default_thread_numbers = get_config_int(section, "default_thread_numbers", config->sys.default_thread_numbers);
    config->sys.max_thread_queue_size = get_config_int(section, "max_thread_queue_size", config->sys.max_thread_queue_size);
    config->sys.send_high_watermark = get_config_int(section, "send_high_watermark", config->sys.send_high_watermark);
    config->sys.send_low_watermark = get_config_int(section, "send_low_watermark", config->sys.send_low_watermark);
    config->sys.ingress_high_watermark = get_config_int(section, "ingress_high_watermark", config->sys.ingress_high_watermark);
    config->sys.ingress_low_watermark = get_config_int(section, "ingress_low_watermark", config->sys.ingress_low_watermark);
    config->sys.enable_transport_telemetry = get_config_bool(section, "enable_transport_telemetry", config->sys.enable_transport_telemetry);
    config->sys.enable_routing_telemetry = get_config_bool(section, "enable_routing_telemetry", config->sys.enable_routing_telemetry);
    config->sys.enforce_encrypted_messages = get_config_bool(section, "enforce_encrypted_messages", config->sys.enforce_encrypted_messages);
    config->sys.require_sender_identity = get_config_bool(section, "require_sender_identity", config->sys.require_sender_identity);
    config->sys.mem_pool_size = get_config_int(section, "mem_pool_size", config->sys.mem_pool_size);
    config->sys.max_queue_size = get_config_int(section, "max_queue_size", config->sys.max_queue_size);
    config->sys.max_node_size = get_config_int(section, "max_node_size", config->sys.max_node_size);
    config->sys.max_transport_buffer_size = get_config_int(section, "max_transport_buffer_size", config->sys.max_transport_buffer_size);
    config->sys.max_timeout = get_config_int(section, "max_timeout", config->sys.max_timeout);
    config->sys.msg_flag = get_config_int(section, "msg_flag", config->sys.msg_flag);
    config->sys.reuse_transport = get_config_bool(section, "reuse_transport", config->sys.reuse_transport);
    config->sys.default_transport_type = get_config_int(section, "default_transport_type", config->sys.default_transport_type);
    config->sys.transport_auto_select = get_config_int(section, "transport_auto_select", config->sys.transport_auto_select);

    printf("[DEBUG] System: threads=%d, memory=%dKB\n",
           config->sys.default_thread_numbers,
           config->sys.mem_pool_size / 1024);
}

/**
 * @brief Load logging configuration section
 * @param cfg libconfig configuration object
 * @param config Configuration structure to populate
 * @details Loads log file path, level, target, size limits, and backup settings
 */
static void load_log_config(config_t* cfg, ZOO_SMB_CONFIG_STRUCT* config)
{
    config_setting_t* section = config_lookup(cfg, "log");
    if (!section)
        return;

    const char* file_path_str = get_config_string(section, "file_path", config->log.file_path);
    safe_string_copy(config->log.file_path, file_path_str, sizeof(config->log.file_path));

    config->log.level = get_config_int(section, "level", config->log.level);
    config->log.target = get_config_int(section, "target", config->log.target);
    config->log.max_file_size = get_config_int(section, "max_file_size", config->log.max_file_size);
    config->log.max_backup_files = get_config_int(section, "max_backup_files", config->log.max_backup_files);

    printf("[DEBUG] Log: %s (level:%d, target:%d)\n",
           config->log.file_path,
           config->log.level,
           config->log.target);
}

/**
 * @brief Load bridge configuration section
 * @param cfg libconfig configuration object
 * @param config Configuration structure to populate
 * @details Loads bridge enable flag, service names, topics, and transport types
 */
static void load_bridge_config(config_t* cfg, ZOO_SMB_CONFIG_STRUCT* config)
{
    config_setting_t* section = config_lookup(cfg, "bridge");
    if (!section)
        return;

    config->bridge.enable_bridge = get_config_bool(section, "enable_bridge", config->bridge.enable_bridge);

    const char* from_service = get_config_string(section, "from_service", config->bridge.from_service);
    safe_string_copy(config->bridge.from_service, from_service, sizeof(config->bridge.from_service));

    const char* to_service = get_config_string(section, "to_service", config->bridge.to_service);
    safe_string_copy(config->bridge.to_service, to_service, sizeof(config->bridge.to_service));

    const char* from_topic = get_config_string(section, "from_topic", config->bridge.from_topic);
    safe_string_copy(config->bridge.from_topic, from_topic, sizeof(config->bridge.from_topic));

    const char* to_topic = get_config_string(section, "to_topic", config->bridge.to_topic);
    safe_string_copy(config->bridge.to_topic, to_topic, sizeof(config->bridge.to_topic));

    config->bridge.from_transport_type = get_config_int(section, "from_transport_type", config->bridge.from_transport_type);
    config->bridge.to_transport_type = get_config_int(section, "to_transport_type", config->bridge.to_transport_type);

    printf("[DEBUG] Bridge: %s\n", config->bridge.enable_bridge ? "enabled" : "disabled");
}

/**
 * @brief Create local machine configuration section in libconfig
 * @param root Root configuration setting
 * @details Creates local_machine section with address, port_min, and port_max
 */
static void create_local_machine_section(config_setting_t* root)
{
    config_setting_t* section = config_setting_add(root, "local_machine", CONFIG_TYPE_GROUP);
    config_setting_set_string(config_setting_add(section, "address", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.local_machine.address);
    config_setting_set_int(config_setting_add(section, "port_min", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.local_machine.port_min);
    config_setting_set_int(config_setting_add(section, "port_max", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.local_machine.port_max);
}

/**
 * @brief Create broadcast configuration section in libconfig
 * @param root Root configuration setting
 * @details Creates broadcast section with address, port, interval, and timeout settings
 */
static void create_broadcast_section(config_setting_t* root)
{
    config_setting_t* section = config_setting_add(root, "broadcast", CONFIG_TYPE_GROUP);
    config_setting_set_string(config_setting_add(section, "address", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.broadcast.address);
    config_setting_set_int(config_setting_add(section, "port", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.broadcast.port);
    config_setting_set_int(config_setting_add(section, "interval_ms", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.broadcast.interval_ms);
    config_setting_set_int(config_setting_add(section, "max_check_timeout_ms", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.broadcast.max_check_timeout_ms);
}

/**
 * @brief Create multicast configuration section in libconfig
 * @param root Root configuration setting
 * @details Creates multicast section with address, port, TTL, interface, and loopback settings
 */
static void create_multicast_section(config_setting_t* root)
{
    config_setting_t* section = config_setting_add(root, "multicast", CONFIG_TYPE_GROUP);
    config_setting_set_string(config_setting_add(section, "address", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.multicast.address);
    config_setting_set_int(config_setting_add(section, "port", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.multicast.port);
    config_setting_set_int(config_setting_add(section, "ttl", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.multicast.ttl);
    config_setting_set_string(config_setting_add(section, "interface", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.multicast.interface);
    config_setting_set_bool(config_setting_add(section, "loopback", CONFIG_TYPE_BOOL),
                            ZOO_SMB_CONFIG_DEFAULT.multicast.loopback);
}

/**
 * @brief Create system configuration section in libconfig
 * @param root Root configuration setting
 * @details Creates sys section with all system-level parameters including memory, threads, etc.
 */
static void create_sys_section(config_setting_t* root)
{
    config_setting_t* section = config_setting_add(root, "sys", CONFIG_TYPE_GROUP);

    config_setting_set_int(config_setting_add(section, "default_thread_numbers", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.default_thread_numbers);
    config_setting_set_int(config_setting_add(section, "max_thread_queue_size", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.max_thread_queue_size);
    config_setting_set_int(config_setting_add(section, "send_high_watermark", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.send_high_watermark);
    config_setting_set_int(config_setting_add(section, "send_low_watermark", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.send_low_watermark);
    config_setting_set_int(config_setting_add(section, "ingress_high_watermark", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.ingress_high_watermark);
    config_setting_set_int(config_setting_add(section, "ingress_low_watermark", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.ingress_low_watermark);
    config_setting_set_bool(config_setting_add(section, "enable_transport_telemetry", CONFIG_TYPE_BOOL),
                            ZOO_SMB_CONFIG_DEFAULT.sys.enable_transport_telemetry);
    config_setting_set_bool(config_setting_add(section, "enable_routing_telemetry", CONFIG_TYPE_BOOL),
                            ZOO_SMB_CONFIG_DEFAULT.sys.enable_routing_telemetry);
    config_setting_set_bool(config_setting_add(section, "enforce_encrypted_messages", CONFIG_TYPE_BOOL),
                            ZOO_SMB_CONFIG_DEFAULT.sys.enforce_encrypted_messages);
    config_setting_set_bool(config_setting_add(section, "require_sender_identity", CONFIG_TYPE_BOOL),
                            ZOO_SMB_CONFIG_DEFAULT.sys.require_sender_identity);
    config_setting_set_int(config_setting_add(section, "mem_pool_size", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.mem_pool_size);
    config_setting_set_int(config_setting_add(section, "max_queue_size", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.max_queue_size);
    config_setting_set_int(config_setting_add(section, "max_node_size", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.max_node_size);
    config_setting_set_int(config_setting_add(section, "max_transport_buffer_size", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.max_transport_buffer_size);
    config_setting_set_int(config_setting_add(section, "max_timeout", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.max_timeout);
    config_setting_set_int(config_setting_add(section, "msg_flag", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.msg_flag);
    config_setting_set_bool(config_setting_add(section, "reuse_transport", CONFIG_TYPE_BOOL),
                            ZOO_SMB_CONFIG_DEFAULT.sys.reuse_transport);
    config_setting_set_int(config_setting_add(section, "default_transport_type", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.default_transport_type);
    config_setting_set_int(config_setting_add(section, "transport_auto_select", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.sys.transport_auto_select);
}

/**
 * @brief Create logging configuration section in libconfig
 * @param root Root configuration setting
 * @details Creates log section with file path, levels, targets, and rotation settings
 */
static void create_log_section(config_setting_t* root)
{
    config_setting_t* section = config_setting_add(root, "log", CONFIG_TYPE_GROUP);

    config_setting_set_string(config_setting_add(section, "file_path", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.log.file_path);
    config_setting_set_int(config_setting_add(section, "level", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.log.level);
    config_setting_set_int(config_setting_add(section, "target", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.log.target);
    config_setting_set_int(config_setting_add(section, "max_file_size", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.log.max_file_size);
    config_setting_set_int(config_setting_add(section, "max_backup_files", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.log.max_backup_files);
}

/**
 * @brief Create bridge configuration section in libconfig
 * @param root Root configuration setting
 * @details Creates bridge section with enable flag, service names, topics, and transport types
 */
static void create_bridge_section(config_setting_t* root)
{
    config_setting_t* section = config_setting_add(root, "bridge", CONFIG_TYPE_GROUP);

    config_setting_set_bool(config_setting_add(section, "enable_bridge", CONFIG_TYPE_BOOL),
                            ZOO_SMB_CONFIG_DEFAULT.bridge.enable_bridge);
    config_setting_set_string(config_setting_add(section, "from_service", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.bridge.from_service);
    config_setting_set_string(config_setting_add(section, "to_service", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.bridge.to_service);
    config_setting_set_string(config_setting_add(section, "from_topic", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.bridge.from_topic);
    config_setting_set_string(config_setting_add(section, "to_topic", CONFIG_TYPE_STRING),
                              ZOO_SMB_CONFIG_DEFAULT.bridge.to_topic);
    config_setting_set_int(config_setting_add(section, "from_transport_type", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.bridge.from_transport_type);
    config_setting_set_int(config_setting_add(section, "to_transport_type", CONFIG_TYPE_INT),
                           ZOO_SMB_CONFIG_DEFAULT.bridge.to_transport_type);
}

/**
 * @brief Load configuration from file using libconfig library
 * @param config Configuration structure to populate
 * @param file_path Path to configuration file
 * @return ZOO_TRUE on success, ZOO_FALSE on error
 * @details Initializes config with defaults, then loads from file. Provides detailed error reporting.
 */
static ZOO_BOOL zoo_smb_config_load_from_file(ZOO_SMB_CONFIG_STRUCT* config, const char* file_path)
{
    if (!config || !file_path)
        return ZOO_FALSE;

    // Initialize with defaults first
    memcpy(config, &ZOO_SMB_CONFIG_DEFAULT, sizeof(ZOO_SMB_CONFIG_STRUCT));

    config_t cfg;
    config_init(&cfg);

    if (config_read_file(&cfg, file_path) != CONFIG_TRUE)
    {
        printf("[ERROR] Failed to read config file %s: %s at line %d\n",
               file_path,
               config_error_text(&cfg),
               config_error_line(&cfg));
        config_destroy(&cfg);
        return ZOO_FALSE;
    }

    printf("[DEBUG] Loading configuration from file: %s\n", file_path);

    // Load all configuration sections atomically
    load_local_machine_config(&cfg, config);
    load_broadcast_config(&cfg, config);
    load_multicast_config(&cfg, config);
    load_sys_config(&cfg, config);
    load_log_config(&cfg, config);
    load_bridge_config(&cfg, config);

    config_destroy(&cfg);
    printf("[INFO] Configuration loaded successfully from: %s\n", file_path);
    return ZOO_TRUE;
}

/**
 * @brief Create default configuration file using libconfig format
 * @param file_path Path where to create the configuration file
 * @return ZOO_TRUE on success, ZOO_FALSE on error
 * @details Creates directory structure if needed, then writes default configuration to file
 */
static ZOO_BOOL create_default_config_file(const char* file_path)
{
    if (!file_path || !create_directories(file_path))
    {
        return ZOO_FALSE;
    }

    config_t cfg;
    config_init(&cfg);

    config_setting_t* root = config_root_setting(&cfg);

    // Create all configuration sections atomically
    create_local_machine_section(root);
    create_broadcast_section(root);
    create_multicast_section(root);
    create_sys_section(root);
    create_log_section(root);
    create_bridge_section(root);

    ZOO_BOOL success = (config_write_file(&cfg, file_path) == CONFIG_TRUE);
    if (!success)
    {
        printf("[ERROR] Failed to write default config file: %s\n", file_path);
    }
    else
    {
        printf("[INFO] Created default configuration file: %s\n", file_path);
    }

    config_destroy(&cfg);
    return success;
}

/**
 * @brief Validate and correct configuration values
 * @return ZOO_SMB_OK on success, error code on validation failure
 * @details Validates port ranges, corrects invalid values, ensures minimum requirements
 */
static ZOO_ERROR_TYPE validate_config(void)
{
    // Port range validation
    if (g_smb_config.local_machine.port_min >= g_smb_config.local_machine.port_max)
    {
        printf("[ERROR] Invalid port range: min=%d, max=%d\n",
               g_smb_config.local_machine.port_min,
               g_smb_config.local_machine.port_max);
        return ZOO_SMB_ERROR_INVALID_PARAM;
    }

    // System configuration corrections
    if (g_smb_config.sys.default_thread_numbers == 0)
    {
        g_smb_config.sys.default_thread_numbers = 1;
    }

    if (g_smb_config.sys.mem_pool_size < 1024)
    {
        g_smb_config.sys.mem_pool_size = 1024;
    }

    if (g_smb_config.sys.send_high_watermark == 0)
    {
        g_smb_config.sys.send_high_watermark = g_smb_config.sys.max_thread_queue_size > 0
                                                 ? g_smb_config.sys.max_thread_queue_size
                                                 : 128;
    }

    if (g_smb_config.sys.send_low_watermark == 0 ||
        g_smb_config.sys.send_low_watermark >= g_smb_config.sys.send_high_watermark)
    {
        g_smb_config.sys.send_low_watermark = g_smb_config.sys.send_high_watermark / 2;
        if (g_smb_config.sys.send_low_watermark == 0)
        {
            g_smb_config.sys.send_low_watermark = 1;
        }
    }

    if (g_smb_config.sys.ingress_high_watermark == 0)
    {
        g_smb_config.sys.ingress_high_watermark = g_smb_config.sys.max_queue_size > 0
                                                    ? g_smb_config.sys.max_queue_size
                                                    : ZOO_SMB_MAX_QUEUE_SIZE;
    }

    if (g_smb_config.sys.ingress_low_watermark == 0 ||
        g_smb_config.sys.ingress_low_watermark >= g_smb_config.sys.ingress_high_watermark)
    {
        g_smb_config.sys.ingress_low_watermark = g_smb_config.sys.ingress_high_watermark / 2;
        if (g_smb_config.sys.ingress_low_watermark == 0)
        {
            g_smb_config.sys.ingress_low_watermark = 1;
        }
    }

    if (g_smb_config.multicast.ttl == 0)
    {
        g_smb_config.multicast.ttl = 1;
    }

    return ZOO_SMB_OK;
}

/**
 * @brief Initialize logging subsystem with current configuration
 * @details Configures logging based on loaded configuration settings
 */
static void initialize_logging(void)
{
    ZOO_LOG_CONFIG_STRUCT log_cfg = {
        .file_path = g_smb_config.log.file_path,
        .max_file_size = g_smb_config.log.max_file_size,
        .max_backup_files = g_smb_config.log.max_backup_files,
        .target = g_smb_config.log.target,
        .level = g_smb_config.log.level,
        .append = ZOO_TRUE,
        .thread_id = ZOO_TRUE,
        .timestamp = ZOO_TRUE,
        .file = ZOO_TRUE,
        .function_name = ZOO_TRUE,
        .color = ZOO_FALSE};
    zoo_log_init(&log_cfg);
}

/**
 * @brief Log complete configuration summary to system log
 * @details Outputs all configuration values for debugging and verification
 */
static void log_config_summary(void)
{
    ZOO_LOG_INFO("========== SMB Configuration Summary ==========");

    // Local machine configuration
    ZOO_LOG_INFO("Local Machine Configuration:");
    ZOO_LOG_INFO("  address: %s", g_smb_config.local_machine.address);
    ZOO_LOG_INFO("  port_min: %d", g_smb_config.local_machine.port_min);
    ZOO_LOG_INFO("  port_max: %d", g_smb_config.local_machine.port_max);

    // Broadcast configuration
    ZOO_LOG_INFO("Broadcast Configuration:");
    ZOO_LOG_INFO("  address: %s", g_smb_config.broadcast.address);
    ZOO_LOG_INFO("  port: %d", g_smb_config.broadcast.port);
    ZOO_LOG_INFO("  interval_ms: %d", g_smb_config.broadcast.interval_ms);
    ZOO_LOG_INFO("  max_check_timeout_ms: %d", g_smb_config.broadcast.max_check_timeout_ms);

    // Multicast configuration
    ZOO_LOG_INFO("Multicast Configuration:");
    ZOO_LOG_INFO("  address: %s", g_smb_config.multicast.address);
    ZOO_LOG_INFO("  port: %d", g_smb_config.multicast.port);
    ZOO_LOG_INFO("  ttl: %d", g_smb_config.multicast.ttl);
    ZOO_LOG_INFO("  interface: %s", g_smb_config.multicast.interface);
    ZOO_LOG_INFO("  loopback: %s", g_smb_config.multicast.loopback ? "ZOO_TRUE" : "ZOO_FALSE");

    // System configuration
    ZOO_LOG_INFO("System Configuration:");
    ZOO_LOG_INFO("  default_thread_numbers: %d", g_smb_config.sys.default_thread_numbers);
    ZOO_LOG_INFO("  max_thread_queue_size: %d", g_smb_config.sys.max_thread_queue_size);
    ZOO_LOG_INFO("  send_high_watermark: %u", g_smb_config.sys.send_high_watermark);
    ZOO_LOG_INFO("  send_low_watermark: %u", g_smb_config.sys.send_low_watermark);
    ZOO_LOG_INFO("  ingress_high_watermark: %u", g_smb_config.sys.ingress_high_watermark);
    ZOO_LOG_INFO("  ingress_low_watermark: %u", g_smb_config.sys.ingress_low_watermark);
    ZOO_LOG_INFO("  enable_transport_telemetry: %s", g_smb_config.sys.enable_transport_telemetry ? "ZOO_TRUE" : "ZOO_FALSE");
    ZOO_LOG_INFO("  enable_routing_telemetry: %s", g_smb_config.sys.enable_routing_telemetry ? "ZOO_TRUE" : "ZOO_FALSE");
    ZOO_LOG_INFO("  enforce_encrypted_messages: %s", g_smb_config.sys.enforce_encrypted_messages ? "ZOO_TRUE" : "ZOO_FALSE");
    ZOO_LOG_INFO("  require_sender_identity: %s", g_smb_config.sys.require_sender_identity ? "ZOO_TRUE" : "ZOO_FALSE");
    ZOO_LOG_INFO("  reuse_transport: %s", g_smb_config.sys.reuse_transport ? "ZOO_TRUE" : "ZOO_FALSE");
    ZOO_LOG_INFO("  mem_pool_size: %d bytes (%d KB)", g_smb_config.sys.mem_pool_size, g_smb_config.sys.mem_pool_size / 1024);
    ZOO_LOG_INFO("  max_queue_size: %d", g_smb_config.sys.max_queue_size);
    ZOO_LOG_INFO("  max_node_size: %d", g_smb_config.sys.max_node_size);
    ZOO_LOG_INFO("  max_timeout: %d ms", g_smb_config.sys.max_timeout);
    ZOO_LOG_INFO("  max_transport_buffer_size: %d bytes (%d KB)",
                     g_smb_config.sys.max_transport_buffer_size,
                     g_smb_config.sys.max_transport_buffer_size / 1024);
    ZOO_LOG_INFO("  msg_flag: %d", g_smb_config.sys.msg_flag);
    ZOO_LOG_INFO("  default_transport_type: %d", g_smb_config.sys.default_transport_type);
    ZOO_LOG_INFO("  transport_auto_select: %d", g_smb_config.sys.transport_auto_select);

    // Log configuration
    ZOO_LOG_INFO("Log Configuration:");
    ZOO_LOG_INFO("  file_path: %s", g_smb_config.log.file_path);
    ZOO_LOG_INFO("  level: %d", g_smb_config.log.level);
    ZOO_LOG_INFO("  target: %d", g_smb_config.log.target);
    ZOO_LOG_INFO("  max_file_size: %zu bytes (%zu KB)",
                     g_smb_config.log.max_file_size,
                     g_smb_config.log.max_file_size / 1024);
    ZOO_LOG_INFO("  max_backup_files: %zu", g_smb_config.log.max_backup_files);

    // Bridge configuration
    ZOO_LOG_INFO("Bridge Configuration:");
    ZOO_LOG_INFO("  enable_bridge: %s", g_smb_config.bridge.enable_bridge ? "ZOO_TRUE" : "ZOO_FALSE");
    ZOO_LOG_INFO("  from_service: %s", g_smb_config.bridge.from_service);
    ZOO_LOG_INFO("  to_service: %s", g_smb_config.bridge.to_service);
    ZOO_LOG_INFO("  from_topic: %s", g_smb_config.bridge.from_topic);
    ZOO_LOG_INFO("  to_topic: %s", g_smb_config.bridge.to_topic);
    ZOO_LOG_INFO("  from_transport_type: %d", g_smb_config.bridge.from_transport_type);
    ZOO_LOG_INFO("  to_transport_type: %d", g_smb_config.bridge.to_transport_type);

    ZOO_LOG_INFO("===============================================");
}

/**
 * @brief Initialize the SMB configuration system
 * @return Pointer to the global configuration structure
 * @details Initializes configuration with defaults, creates config file if needed,
 *          loads from file, initializes logging, and validates settings
 * @note This function is idempotent - safe to call multiple times
 */
const ZOO_SMB_CONFIG_STRUCT* zoo_smb_config_init(void)
{
    if (g_config_initialized)
    {
        return &g_smb_config;
    }

    printf("[DEBUG] Starting SMB configuration initialization\n");

    // Load defaults
    memcpy(&g_smb_config, &ZOO_SMB_CONFIG_DEFAULT, sizeof(ZOO_SMB_CONFIG_STRUCT));

    // Handle config file
    if (!file_exists(SMB_CONFIG_FILE_PATH))
    {
        printf("[INFO] Creating default configuration file: %s\n", SMB_CONFIG_FILE_PATH);
        create_default_config_file(SMB_CONFIG_FILE_PATH);
    }

    // Load from file
    zoo_smb_config_load_from_file(&g_smb_config, SMB_CONFIG_FILE_PATH);

    // Initialize logging and validate
    initialize_logging();
    validate_config();
    g_config_initialized = ZOO_TRUE;

    // Log summary
    log_config_summary();
    ZOO_LOG_INFO("Configuration initialization completed successfully");

    return &g_smb_config;
}

/**
 * @brief Get the current SMB configuration
 * @return Pointer to the global configuration structure
 * @details Returns current configuration, initializing if necessary
 * @note Thread-safe for read access after initialization
 */
const ZOO_SMB_CONFIG_STRUCT* zoo_smb_get_config(void)
{
    if (!g_config_initialized)
    {
        zoo_smb_config_init();
    }
    return &g_smb_config;
}

/**
 * @brief Save configuration to file using libconfig format
 * @param config Configuration structure to save
 * @param file_path Path where to save the configuration
 * @return ZOO_TRUE on success, ZOO_FALSE on error
 * @details Currently creates a default configuration file regardless of input config
 * @todo (smb-config) Implement persistence of caller-provided config values instead of defaults.
 */
ZOO_BOOL zoo_smb_config_save_to_file(const ZOO_SMB_CONFIG_STRUCT* config, const char* file_path)
{
    if (!config || !file_path)
        return ZOO_FALSE;
    return create_default_config_file(file_path);
}

/**
 * @brief Reload configuration from file
 * @param file_path Path to configuration file (NULL to use default)
 * @return ZOO_SMB_OK on success, error code on failure
 * @details Loads configuration from file, validates it, and updates global config.
 *          Rolls back to previous configuration on validation failure.
 */
ZOO_ERROR_TYPE zoo_smb_config_reload(const char* file_path)
{
    const char* config_file = file_path ? file_path : SMB_CONFIG_FILE_PATH;

    ZOO_SMB_CONFIG_STRUCT temp_config;
    if (!zoo_smb_config_load_from_file(&temp_config, config_file))
    {
        ZOO_LOG_ERROR("Failed to reload configuration from file: %s", config_file);
        return ZOO_SMB_ERROR_CONFIG_FILE_READ_FAILED;
    }

    ZOO_SMB_CONFIG_STRUCT old_config = g_smb_config;
    g_smb_config = temp_config;

    ZOO_ERROR_TYPE result = validate_config();
    if (result != ZOO_SMB_OK)
    {
        g_smb_config = old_config;
        return result;
    }

    ZOO_LOG_INFO("Configuration reloaded successfully");
    return ZOO_SMB_OK;
}