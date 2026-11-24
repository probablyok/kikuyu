#ifndef GUI_MODE_H
#define GUI_MODE_H

#include "cli_args.h"
#include "cpu_state.h"

/// @brief Program flow after setup for GUI mode
/// @param cmd_args The processed command arguments from startup
/// @param cpu The emulated 6502 CPU
void gui_run(const CmdArgs* cmd_args, Cpu* cpu);

#endif // GUI_MODE_H
