#ifndef CLI_MODE_H
#define CLI_MODE_H

#include "cli_args.h"
#include "cpu_state.h"

/// @brief Enum for various representing valid debug commands
typedef enum {
    CMD_STEP,
    CMD_CYCLE,
    CMD_PRINT,
    CMD_MEM,
    CMD_HELP,
    CMD_QUIT,
    CMD_IRQ,
    CMD_NMI,
    CMD_RESET,
} DebugCmdType;

typedef struct {
    DebugCmdType type;
    union {
        CpuWord addr;
    } arg;
} DebugCmd;

/// @brief Program flow after setup for CLI mode
/// @param cmd_args The processed command arguments from startup
/// @param cpu The emulated 6502 CPU
void cli_run(const CmdArgs* cmd_args, Cpu* cpu);

/// @brief Handler for processing valid debug commands with emulator
/// @param cmd The debug command to be handled
/// @param cpu The emulated 6502 CPU
/// @param stepping Flag for whether the debugger should keep ticking until CPU sync signal is
/// `true` after handling
/// @return `false` for commands that do not tick the CPU in anyway, `true` otherwise
bool debug_cmd_handler(DebugCmd cmd, Cpu* cpu, bool* skip);

/// @brief Handler for reading user input in debug mode
/// @param last The last valid debug command from previous input
/// @param return Debug command to used based on user input
DebugCmd debug_read_cmd(DebugCmd last);

#endif // CLI_MODE_H
