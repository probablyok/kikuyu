#include "cpu_macros.h"
#include "cpu_state.h"
#include "safe_wrappers.h"

#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/// @brief Enum for various representing valid debug commands
typedef enum { CMD_STEP, CMD_CYCLE, CMD_MEM, CMD_HELP, CMD_QUIT } DebugCmdType;

typedef struct {
    DebugCmdType type;
    union {
        CpuWord addr;
    } arg;
} DebugCmd;

/// @brief Struct for encapsulating simulation parameters set from command line
typedef struct {
    char* bin_file;  // Filepath to file representing assembled 6502 code
    long bin_offset; // Offset into CPU memory for placing binary file contents (given in base 16)
    long num_cycles; // The number of cycles to run the simulation for after resetting (given in
                     // base 10)
    bool is_debug;   // Whether to run in debug mode or not
} CmdArgs;

// Alias to avoid verbosity
typedef struct option Option;

/// @brief Helper for displaying the helper message to avoid redundancy
/// @param prog_name Name of the program. Extract if with `argv[0]`
/// @param steam The output buffer being written to (usually `stderr` or `stdout`)
void print_usage(char* prog_name, FILE* stream) {
    fprintf(stream, "Usage: %s [options] bin_file\n", prog_name);
}

/// @brief Handler for processing and setting up emulator with CLI args
/// @param argc The number of CLI args
/// @param argv List of character buffers with CLI args in order
//  @param cmd_args Structure holding processed arguments for emulator configuration
void handle_args(int argc, char** argv, CmdArgs* cmd_args) {
    // Parse command line arguments
    int opt;
    int opt_idx = 0;

    Option long_options[] = {
        {"offset", required_argument, 0, 'o'},
        {"cycles", required_argument, 0, 'c'},
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    // Set defaults for processed cmd args
    cmd_args->bin_file = NULL;
    cmd_args->bin_offset = 0x8000;
    cmd_args->num_cycles = 100;
    cmd_args->is_debug = false;

    // For converting strings
    char* endptr;
    long cycles, offset;

    // Check all CLI args
    while ((opt = getopt_long(argc, argv, "o:c:dh", long_options, &opt_idx)) != -1) {
        endptr = NULL;
        switch (opt) {
        case 'o':
            // Convert the string rep. of cycles to long
            offset = strtol(optarg, &endptr, 16);
            if (optarg && *optarg != '\0' && *endptr == '\0') {
                // Conversion successful
                cmd_args->bin_offset = offset;
                break;
            }
            fprintf(stderr, "Error: Invalid hex offset '%s'\n", optarg);
            exit(EXIT_FAILURE);
        case 'c':
            // Convert the string rep. of cycles to long
            cycles = strtol(optarg, &endptr, 10);
            if (optarg && *optarg != '\0' && *endptr == '\0') {
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
        case 'h':
            print_usage(argv[0], stdout);
            printf("Simple NMOS 6502 emulator written in C\n\n");
            printf("-o, --offset\n\tspecifies the offset into CPU memory (in base 16) where the "
                   "program contents are loaded\n");
            printf("-c, --cycles\n\tspecifies the number of cycles (in base 10) the emulator runs "
                   "for before termination\n");
            printf("-d, --debug\n\tset the emulator to run in debug mode\n");
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

/// @brief Handler for processing valid debug commands with emulator
/// @param cmd The debug command to be handled
/// @param cpu The emulated 6502 CPU
/// @param stepping Flag for whether the debugger should keep ticking until CPU sync signal is
/// `true` after handling
/// @return `false` for commands that do not tick the CPU in anyway, `true` otherwise
bool debug_cmd_handler(DebugCmd cmd, Cpu* cpu, bool* skip) {
    switch (cmd.type) {
    case CMD_HELP:
        printf("Debug mode commands:\n"
               "  h, help         Print this command\n"
               "  c, cycle        Step the emulator by one tick/cycle\n"
               "  s, step         Step the emulator by one instruction\n"
               "  m, mem <hex>    Read byte at hex address in CPU memory\n"
               "  q, quit         Quit the emulator\n");
        return false;

    case CMD_CYCLE:
        *skip = false;
        return true;

    case CMD_STEP:
        *skip = true;
        return true;

    case CMD_MEM:
        printf("$%04X: $%02X\n", cmd.arg.addr, cpu->mem[cmd.arg.addr]);
        return false;

    case CMD_QUIT:
        return true;
    }

    return false;
}

/// @brief Handler for reading user input in debug mode
/// @param last The last valid debug command from previous input
/// @param return Debug command to used based on user input
DebugCmd debug_read_cmd(DebugCmd last) {
    while (true) {
        printf("> ");
        fflush(stdout);

        size_t len, cap;
        char* line = safe_read_line(stdin, &len, &cap);
        if (line == NULL) {
            printf("\n");
            return (DebugCmd){.type = CMD_QUIT};
        }

        // Strip tailing newline
        line[strcspn(line, "\n")] = '\0';
        char* verb = strtok(line, " ");

        // Empty input repeats last valid command with operands
        if (verb == NULL) {
            free(line);
            return last;
        }

        if (!strcasecmp(verb, "q") || !strcasecmp(verb, "quit")) {
            free(line);
            return (DebugCmd){.type = CMD_QUIT};
        }

        if (!strcasecmp(verb, "s") || !strcasecmp(verb, "step")) {
            free(line);
            return (DebugCmd){.type = CMD_STEP};
        }

        if (!strcasecmp(verb, "c") || !strcasecmp(verb, "cycle")) {
            free(line);
            return (DebugCmd){.type = CMD_CYCLE};
        }

        if (!strcasecmp(verb, "h") || !strcasecmp(verb, "help")) {
            free(line);
            return (DebugCmd){.type = CMD_HELP};
        }

        if (!strcasecmp(verb, "m") || !strcasecmp(verb, "mem")) {
            char* arg = strtok(NULL, " ");

            char* endptr = NULL;
            long val = arg ? strtol(arg, &endptr, 16) : -1;

            // Ignore invalid arguments
            if (arg && *arg != '\0' && *endptr == '\0' && val >= 0 && val <= 0xFFFF) {
                free(line);
                return (DebugCmd){.type = CMD_MEM, .arg.addr = (uint16_t)val};
            }
            printf("Usage: mem <hex_addr> (0000-FFFF)\n");
            free(line);
            continue;
        }

        // Fall through to invalid command
        printf("Invalid command: %s\n", verb);
        free(line);
    }
}

int main(int argc, char** argv) {
    // Get emulation parameters
    CmdArgs cmd_args;
    handle_args(argc, argv, &cmd_args);

    // Create CPU
    Cpu cpu = {0};

    // Load ROM into CPU
    if (safe_read_file_bin(cmd_args.bin_file, &cpu, cmd_args.bin_offset)) {
        exit(EXIT_FAILURE); // Crash on I/O error
    }

    // Set hardware vectors ONLY IF binary file doesn't overwrite them
    if (cpu.mem[CPU_MEM_RESET_VEC_ADDR_L] == 0x00 && cpu.mem[CPU_MEM_RESET_VEC_ADDR_H] == 0x00) {
        cpu.mem[CPU_MEM_RESET_VEC_ADDR_L] = cmd_args.bin_offset & 0xFF;
        cpu.mem[CPU_MEM_RESET_VEC_ADDR_H] = cmd_args.bin_offset >> 8;
    }

    // Boot CPU
    cpu_reset(&cpu);

    DebugCmd last = {.type = CMD_STEP};
    bool skip = false;

    // Tick CPU for user specified number of cycles
    for (long n = 0; n < cmd_args.num_cycles; n++) {
        // Just tick CPU until opcode is finished executing
        if (!skip) {
            printf("Cycle %ld:\n", n); // Current cycle
            cpu_disp_state(&cpu);      // CPU state

            // Process arguments in debug mode
            if (cmd_args.is_debug) {
                DebugCmd debug_cmd;
                do {
                    debug_cmd = debug_read_cmd(last);
                    // Special handling for QUIT
                    if (debug_cmd.type == CMD_QUIT) {
                        printf("Quitting...\n");
                        exit(EXIT_SUCCESS);
                    }
                    last = debug_cmd;
                } while (!debug_cmd_handler(debug_cmd, &cpu, &skip));
            }
        }

        cpu_tick(&cpu); // Tick the CPU

        // Opcode finished executing
        if (skip && cpu.sync) {
            skip = false;
        }
    }

    return 0;
}
