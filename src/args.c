#include "args.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

// Alias to avoid verbosity
typedef struct option Option;

/// @brief Helper for displaying the helper message to avoid redundancy
/// @param prog_name Name of the program. Extract if with `argv[0]`
/// @param steam The output buffer being written to (usually `stderr` or `stdout`)
static void print_usage(const char* prog_name, FILE* stream) {
    fprintf(stream, "Usage: %s [options] bin_file\n", prog_name);
}

/// @brief Helper for parsing a hex offset for -o with error handling
/// @param arg The hex offset being converted to a long
/// @return The parsed hex offset
static long parse_hex_offset(const char* arg) {
    // Convert the string rep. of cycles to long
    char* endptr = NULL;
    errno = 0;
    long offset = strtol(arg, &endptr, 16);
    if (arg && *arg != '\0' && *endptr == '\0' && errno != ERANGE && offset >= 0 &&
        offset <= 0xFFFF) {
        // Conversion successful
        return offset;
    }
    fprintf(stderr, "Error: Invalid hex offset '%s'\n", arg);
    exit(EXIT_FAILURE);
}

static void init_defaults(CmdArgs* cmd_args) {
    // Set defaults for processed cmd args
    cmd_args->bin_file = NULL;
    cmd_args->bin_offset = 0;
    cmd_args->num_cycles = 100;
    cmd_args->is_debug = false;
    cmd_args->is_infinite = false;
}

void handle_cli_args(int argc, char** argv, CmdArgs* cmd_args) {
    // Parse command line arguments (CLI mode)
    int opt;
    int opt_idx = 0;

    Option long_options[] = {
        {"offset", required_argument, 0, 'o'},
        {"cycles", required_argument, 0, 'c'},
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    init_defaults(cmd_args);

    // For converting strings
    char* endptr;
    long cycles;

    // Check all CLI args
    while ((opt = getopt_long(argc, argv, "o:c:dh", long_options, &opt_idx)) != -1) {
        endptr = NULL;
        switch (opt) {
        case 'o':
            cmd_args->bin_offset = parse_hex_offset(optarg);
            break;
        case 'c':
            // Check if user specified infinite cycles
            if (optarg && !strcasecmp(optarg, "inf")) {
                cmd_args->is_infinite = true;
                break;
            }

            // Convert the string rep. of cycles to long
            errno = 0;
            cycles = strtol(optarg, &endptr, 10);
            if (optarg && *optarg != '\0' && *endptr == '\0' && errno != ERANGE && cycles >= 0) {
                // Conversion successful
                cmd_args->num_cycles = cycles;
                break;
            }
            fprintf(stderr, "Error: Invalid cycle count '%s'\n", optarg);
            exit(EXIT_FAILURE);
        case 'd':
            cmd_args->is_debug = true;
            break;
        case 'h':
            print_usage(argv[0], stdout);
            printf("Simple NMOS 6502 emulator written in C\n\n");
            printf("-o, --offset\n  Specifies the offset into CPU memory (in base 16) where the "
                   "program contents are loaded. Offset is 0x0000 if not specified.\n");
            printf(
                "-c, --cycles\n  Specifies the number of cycles (in base 10) the emulator runs "
                "for before termination. Specify \"inf\" to run indefinitely. "
                "Emulation stops after 100 cycles if not specified. Has no effect in GUI mode.\n");
            printf("-d, --debug\n  Set the emulator to run in debug mode. Has no effect in GUI "
                   "mode, and off by default.\n");
            exit(EXIT_SUCCESS);
        default:
            print_usage(argv[0], stderr);
            exit(EXIT_FAILURE);
        }
    }

    // Get binary file
    if (optind < argc) {
        cmd_args->bin_file = argv[optind];
    } else {
        fprintf(stderr, "Error: Missing required binary file\n");
        print_usage(argv[0], stderr);
        exit(EXIT_FAILURE);
    }
}

void handle_gui_args(int argc, char** argv, CmdArgs* cmd_args) {
    // Parse command line arguments (GUI mode)
    int opt;
    int opt_idx = 0;

    Option long_options[] = {
        {"offset", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    init_defaults(cmd_args);

    // Check all GUI args
    while ((opt = getopt_long(argc, argv, "o:h", long_options, &opt_idx)) != -1) {
        switch (opt) {
        case 'o':
            cmd_args->bin_offset = parse_hex_offset(optarg);
            break;

        case 'h':
            print_usage(argv[0], stdout);
            printf("Simple NMOS 6502 emulator (GUI)\n\n");
            printf("-o, --offset\n  Specifies the offset into CPU memory (in base 16) where the "
                   "program contents are loaded. Offset is 0x0000 if not specified.\n");
            exit(EXIT_SUCCESS);

        default:
            print_usage(argv[0], stderr);
            exit(EXIT_FAILURE);
        }
    }

    // Get binary file
    if (optind < argc) {
        cmd_args->bin_file = argv[optind];
    } else {
        fprintf(stderr, "Error: Missing required binary file\n");
        print_usage(argv[0], stderr);
        exit(EXIT_FAILURE);
    }
}
