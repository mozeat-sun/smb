#ifndef LIBCONFIG_STUB_H
#define LIBCONFIG_STUB_H

/*
 * Minimal libconfig compatibility shim for environments where libconfig
 * headers are unavailable at build time.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_TRUE 1
#define CONFIG_FALSE 0

#define CONFIG_TYPE_GROUP 1
#define CONFIG_TYPE_STRING 2
#define CONFIG_TYPE_INT 3
#define CONFIG_TYPE_BOOL 4

typedef struct config_stub_struct
{
    int dummy;
} config_t;

typedef struct config_setting_stub_struct
{
    int dummy;
} config_setting_t;

/**
 * @brief Initializes a stub libconfig context.
 *
 * @param cfg Stub configuration object.
 */
static inline void config_init(config_t* cfg)
{
    (void)cfg;
}

static inline void config_destroy(config_t* cfg)
{
    (void)cfg;
}

static inline int config_read_file(config_t* cfg, const char* path)
{
    (void)cfg;
    (void)path;
    return CONFIG_FALSE;
}

static inline const char* config_error_text(const config_t* cfg)
{
    (void)cfg;
    return "libconfig unavailable";
}

static inline int config_error_line(const config_t* cfg)
{
    (void)cfg;
    return 0;
}

static inline config_setting_t* config_lookup(const config_t* cfg, const char* path)
{
    (void)cfg;
    (void)path;
    return (config_setting_t*)0;
}

static inline int config_setting_lookup_string(const config_setting_t* setting, const char* name, const char** value)
{
    (void)setting;
    (void)name;
    (void)value;
    return CONFIG_FALSE;
}

static inline int config_setting_lookup_int(const config_setting_t* setting, const char* name, int* value)
{
    (void)setting;
    (void)name;
    (void)value;
    return CONFIG_FALSE;
}

static inline int config_setting_lookup_bool(const config_setting_t* setting, const char* name, int* value)
{
    (void)setting;
    (void)name;
    (void)value;
    return CONFIG_FALSE;
}

static inline config_setting_t* config_root_setting(config_t* cfg)
{
    (void)cfg;
    return (config_setting_t*)0;
}

static inline config_setting_t* config_setting_add(config_setting_t* parent, const char* name, int type)
{
    (void)parent;
    (void)name;
    (void)type;
    return (config_setting_t*)0;
}

static inline int config_setting_set_string(config_setting_t* setting, const char* value)
{
    (void)setting;
    (void)value;
    return CONFIG_TRUE;
}

static inline int config_setting_set_int(config_setting_t* setting, int value)
{
    (void)setting;
    (void)value;
    return CONFIG_TRUE;
}

static inline int config_setting_set_bool(config_setting_t* setting, int value)
{
    (void)setting;
    (void)value;
    return CONFIG_TRUE;
}

static inline int config_write_file(config_t* cfg, const char* path)
{
    (void)cfg;
    (void)path;
    return CONFIG_FALSE;
}

#ifdef __cplusplus
}
#endif

#endif