#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

// Command line options structure
typedef struct {
    const char* target;
    const char* topic;
    const char* payload;
    int log_level;
    int transport_type;
} COMMAND_OPTIONS_STRUCT;

// Parse command line arguments (legacy function name)
int parse_arguments(int argc, char* argv[], COMMAND_OPTIONS_STRUCT* options);

// Simple command line argument parser
typedef struct {
    const char* name;
    const char* address;
    int port;
    bool verbose;
} parsed_args_t;

// Parse command line arguments
int parse_args(int argc, char* argv[], parsed_args_t* args);

// Print usage information
void print_usage(const char* program_name);

#endif // PARSER_H
