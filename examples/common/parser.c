#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_client_server_usage(const char* program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -t, --target <name>     Target server name (default: 127.0.0.1:8080)\n");
    printf("  --topic <topic>         Message topic (default: default_topic)\n");
    printf("  -p, --payload <text>    Payload text (default: Hello, World!)\n");
    printf("  -l, --log-level <0-6>   TRACE..OFF (default: 2)\n");
    printf("  --transport <type>      0=UDP, 1=TCP, 2=UDP_BROADCAST, 3=UDP_MULTICAST, 4=SHM\n");
    printf("  -h, --help              Show this help message\n");
}

// Parse command line arguments (legacy function name)
/**
 * @brief Test or example function parse_arguments.
 */
int parse_arguments(int argc, char* argv[], COMMAND_OPTIONS_STRUCT* options) {
    // Initialize defaults
    options->target = "127.0.0.1:8080";
    options->topic = "default_topic";
    options->payload = "Hello, World!";
    options->log_level = 2; // INFO level
    options->transport_type = 0; // Default to UDP
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--target") == 0) {
            if (i + 1 < argc) {
                options->target = argv[++i];
            }
        } else if (strcmp(argv[i], "--topic") == 0) {
            if (i + 1 < argc) {
                options->topic = argv[++i];
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--payload") == 0) {
            if (i + 1 < argc) {
                options->payload = argv[++i];
            }
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log-level") == 0) {
            if (i + 1 < argc) {
                options->log_level = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--transport") == 0) {
            if (i + 1 < argc) {
                options->transport_type = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_client_server_usage(argv[0]);
            return 1; // Indicate help was requested
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_client_server_usage(argv[0]);
            return -1;
        }
    }
    
    return 0;
}

// Parse command line arguments
/**
 * @brief Test or example function parse_args.
 */
int parse_args(int argc, char* argv[], parsed_args_t* args) {
    // Initialize defaults
    args->name = "default";
    args->address = "127.0.0.1";
    args->port = 8080;
    args->verbose = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--name") == 0) {
            if (i + 1 < argc) {
                args->name = argv[++i];
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--address") == 0) {
            if (i + 1 < argc) {
                args->address = argv[++i];
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                args->port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            args->verbose = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return -1; // Indicate help was requested
        }
    }
    
    return 0;
}

// Print usage information
/**
 * @brief Test or example function print_usage.
 */
void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -n, --name <name>        Service name (default: default)\n");
    printf("  -a, --address <address>  Server address (default: 127.0.0.1)\n");
    printf("  -p, --port <port>        Server port (default: 8080)\n");
    printf("  -v, --verbose            Enable verbose output\n");
    printf("  -h, --help               Show this help message\n");
}
