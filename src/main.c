#include "cli_args.h"
#include "cli_mode.h"
#include "gui_mode.h"
#include "safe_wrappers.h"

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

    // Different handler for GUI
    if (cmd_args.is_gui) {
        gui_run(&cmd_args, &cpu);
        return 0;
    }

    cli_run(&cmd_args, &cpu);
    return 0;
}
