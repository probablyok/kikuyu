#include "cli_args.h"

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <strings.h>

// Alias to avoid verbosity
typedef struct option Option;

void print_usage(const char* prog_name, FILE* stream) {
    fprintf(stream, "Usage: %s [options] bin_file\n", prog_name);
}

void handle_args(int argc, char** argv, CmdArgs* cmd_args) {
    // Parse command line arguments
    int opt;
    int opt_idx = 0;

    Option long_options[] = {
        {"offset", required_argument, 0, 'o'}, {"cycles", required_argument, 0, 'c'},
        {"debug", no_argument, 0, 'd'},        {"gui", no_argument, 0, 'g'},
        {"help", no_argument, 0, 'h'},         {0, 0, 0, 0},
    };

    // Set defaults for processed cmd args
    cmd_args->bin_file = NULL;
    cmd_args->bin_offset = 0;
    cmd_args->num_cycles = 100;
    cmd_args->is_debug = false;
    cmd_args->is_infinite = false;
    cmd_args->is_gui = false;

    // For converting strings
    char* endptr;
    long cycles, offset;

    // Check all CLI args
    while ((opt = getopt_long(argc, argv, "o:c:dgh", long_options, &opt_idx)) != -1) {
        endptr = NULL;
        switch (opt) {
        case 'o':
            // Convert the string rep. of cycles to long
            errno = 0;
            offset = strtol(optarg, &endptr, 16);
            if (optarg && *optarg != '\0' && *endptr == '\0' && errno != ERANGE) {
                // Conversion successful
                cmd_args->bin_offset = offset;
                break;
            }
            fprintf(stderr, "Error: Invalid hex offset '%s'\n", optarg);
            exit(EXIT_FAILURE);
        case 'c':
            // Check if user specified infinite cycles
            if (optarg && !strcasecmp(optarg, "inf")) {
                cmd_args->is_infinite = true;
                break;
            }

            // Convert the string rep. of cycles to long
            errno = 0;
            cycles = strtol(optarg, &endptr, 10);
            if (optarg && *optarg != '\0' && *endptr == '\0' && errno != ERANGE && cycles > 0) {
                // Conversion successful
                cmd_args->num_cycles = cycles;
                break;
            }
            fprintf(stderr, "Error: Invalid cycle count '%s'\n", optarg);
            exit(EXIT_FAILURE);
            break;
        case 'd':
            cmd_args->is_debug = true;
            break;
        case 'g':
            cmd_args->is_gui = true;
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
            printf("-g, --gui\n  Launch the emulator with GUI rendering. CLI mode is used if not "
                   "specified\n");
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
