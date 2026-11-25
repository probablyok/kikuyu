#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

/// @brief Struct for encapsulating simulation parameters set from command line
typedef struct {
    char* bin_file;   // Filepath to file representing assembled 6502 code
    long bin_offset;  // Offset into CPU memory for placing binary file contents (given in base 16)
    long num_cycles;  // The number of cycles to run the simulation for after resetting (given in
                      // base 10)
    bool is_debug;    // Whether to run in debug mode or not
    bool is_infinite; // Whether to run simulation indefinitely
} CmdArgs;

/// @brief Handler for processing and setting up emulator with command-line args (CLI mode)
/// @param argc The number of command-line args
/// @param argv List of character buffers with command-line args in order
//  @param cmd_args Structure holding processed arguments for emulator configuration
void handle_cli_args(int argc, char** argv, CmdArgs* cmd_args);

/// @brief Handler for processing and setting up emulator with command-line args (GUI mode)
/// @param argc The number of command-line args
/// @param argv List of character buffers with command-line args in order
//  @param cmd_args Structure holding processed arguments for emulator configuration
void handle_gui_args(int argc, char** argv, CmdArgs* cmd_args);

#endif // CLI_ARGS_H
