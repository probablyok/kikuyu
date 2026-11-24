#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <stdbool.h>
#include <stdio.h>

/// @brief Struct for encapsulating simulation parameters set from command line
typedef struct {
    char* bin_file;   // Filepath to file representing assembled 6502 code
    long bin_offset;  // Offset into CPU memory for placing binary file contents (given in base 16)
    long num_cycles;  // The number of cycles to run the simulation for after resetting (given in
                      // base 10)
    bool is_debug;    // Whether to run in debug mode or not
    bool is_infinite; // Whether to run simulation indefinitely
    bool is_gui;      // Whether to launch in GUI mode
} CmdArgs;

/// @brief Helper for displaying the helper message to avoid redundancy
/// @param prog_name Name of the program. Extract if with `argv[0]`
/// @param steam The output buffer being written to (usually `stderr` or `stdout`)
void print_usage(const char* prog_name, FILE* stream);

/// @brief Handler for processing and setting up emulator with CLI args
/// @param argc The number of CLI args
/// @param argv List of character buffers with CLI args in order
//  @param cmd_args Structure holding processed arguments for emulator configuration
void handle_args(int argc, char** argv, CmdArgs* cmd_args);

#endif // CLI_ARGS_H
