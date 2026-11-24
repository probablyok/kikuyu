#include "raygui.h"
#include "raylib.h"

#include "cpu_addr.h"
#include "cpu_macros.h"
#include "cpu_state.h"
#include "layout_helper.h"
#include "safe_wrappers.h"

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/// @brief Enum for various representing valid debug commands
typedef enum {
    CMD_STEP,
    CMD_CYCLE,
    CMD_PRINT,
    CMD_MEM,
    CMD_HELP,
    CMD_QUIT,
    CMD_IRQ,
    CMD_NMI
} DebugCmdType;

typedef struct {
    DebugCmdType type;
    union {
        CpuWord addr;
    } arg;
} DebugCmd;

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
               "  p, print        Print the CPU state\n"
               "  m, mem <hex>    Read byte at hex address in CPU memory\n"
               "  n, nmi          Assert external NMI line\n"
               "  i, irq          Trigger external IRQ line\n"
               "  q, quit         Quit the emulator\n");
        return false;

    case CMD_CYCLE:
        *skip = false;
        return true;

    case CMD_STEP:
        *skip = true;
        return true;

    case CMD_PRINT:
        cpu_disp_state(cpu);
        return false;

    case CMD_MEM:
        printf("$%04X: $%02X\n", cmd.arg.addr, cpu->mem[cmd.arg.addr]);
        return false;

    case CMD_IRQ:
        cpu->irq_pending = true;
        if (cpu->sr & CPU_FLAG_I) {
            printf("IRQ asserted (masked by I=1)\n");
        } else {
            printf("IRQ asserted\n");
        }
        return false;

    case CMD_NMI:
        cpu->nmi_pending = true;
        printf("NMI triggered\n");
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
        line[strcspn(line, "\n\r")] = '\0';
        char* verb = strtok(line, " \t");

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

        if (!strcasecmp(verb, "p") || !strcasecmp(verb, "print")) {
            free(line);
            return (DebugCmd){.type = CMD_PRINT};
        }

        if (!strcasecmp(verb, "h") || !strcasecmp(verb, "help")) {
            free(line);
            return (DebugCmd){.type = CMD_HELP};
        }

        if (!strcasecmp(verb, "m") || !strcasecmp(verb, "mem")) {
            char* arg = strtok(NULL, " \t");

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

        if (!strcasecmp(verb, "n") || !strcasecmp(verb, "nmi")) {
            free(line);
            return (DebugCmd){.type = CMD_NMI};
        }

        if (!strcasecmp(verb, "i") || !strcasecmp(verb, "irq")) {
            free(line);
            return (DebugCmd){.type = CMD_IRQ};
        }

        // Fall through to invalid command
        printf("Invalid command: %s\n", verb);
        free(line);
    }
}

/// @brief Program flow after setup for CLI mode
/// @param cmd_args The processed command arguments from startup
/// @param cpu The emulated 6502 CPU
void CLI_handler(CmdArgs* cmd_args, Cpu* cpu) {
    DebugCmd last = {.type = CMD_STEP};
    bool skip = false;

    // Tick CPU for user specified number of cycles
    for (uint64_t n = 0; n < (uint64_t)cmd_args->num_cycles || cmd_args->is_infinite; n++) {
        // Just tick CPU until opcode is finished executing
        if (!skip) {
            printf("Cycle %llu:\n", (unsigned long long)n); // Current cycle
            cpu_disp_state(cpu);                            // CPU state

            // Process arguments in debug mode
            if (cmd_args->is_debug) {
                DebugCmd debug_cmd;
                do {
                    debug_cmd = debug_read_cmd(last);
                    // Special handling for QUIT
                    if (debug_cmd.type == CMD_QUIT) {
                        printf("Quitting...\n");
                        exit(EXIT_SUCCESS);
                    }
                    last = debug_cmd;
                } while (!debug_cmd_handler(debug_cmd, cpu, &skip));
            }
        }

        cpu_tick(cpu); // Tick the CPU

        // Opcode finished executing
        if (skip && cpu->cur_step == cpu_fetch_opcode) {
            skip = false;
        }
    }
}

/// @brief Program flow after setup for GUI mode
/// @param cmd_args The processed command arguments from startup
/// @param cpu The emulated 6502 CPU
void GUI_handler(CmdArgs* cmd_args, Cpu* cpu) {
    InitWindow(900, 600, "Kikuyu 6502 Emulator");
    SetTargetFPS(60);

    bool running = false;
    float cycles_per_frame = 1.0f;
    uint64_t total_cycles = 0;
    int mem_page = (int)(cmd_args->bin_offset >> 8) & 0xFF;

    while (!WindowShouldClose()) {
        // Tick the CPU for the number of cycles per frame
        if (running) {
            int steps = (int)cycles_per_frame;
            for (int i = 0; i < steps; i++) {
                cpu_tick(cpu);
                total_cycles++;
            }
        }

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();
        float top_h = 50.0f;
        float left_w = sw * 0.28f;

        // Render the top toolbar (Control)
        Layout top = layout_box((Rectangle){0, 0, sw, top_h}, 8);
        GuiToggle(layout_next(&top, 80, 30), running ? "Pause" : "Run", &running);
        if (GuiButton(layout_next(&top, 90, 30), "Step Inst") && !running) {
            do {
                cpu_tick(cpu);
                total_cycles++;
            } while (cpu->cur_step != cpu_fetch_opcode);
        }
        if (GuiButton(layout_next(&top, 90, 30), "Step Cycle") && !running) {
            cpu_tick(cpu);
            total_cycles++;
        };
        if (GuiButton(layout_next(&top, 70, 30), "Reset")) {
            running = false;
            cpu_reset(cpu);
            total_cycles = 0;
        }
        if (GuiButton(layout_next(&top, 55, 30), "IRQ"))
            cpu->irq_pending = true;
        if (GuiButton(layout_next(&top, 55, 30), "NMI"))
            cpu->nmi_pending = true;

        GuiSliderBar(layout_next(&top, 260, 30), "", TextFormat("%d cpf", (int)cycles_per_frame),
                     &cycles_per_frame, 1.0f, 100.0f);

        // Render the left panel (CPU Registers / Status / Buses)
        Rectangle stuats_box = {8, top_h, left_w - 12, sh - top_h - 8};
        GuiGroupBox(stuats_box, "Registers");
        Layout status = layout_box(stuats_box, 12);
        layout_row(&status, 0);

        GuiLabel(layout_row(&status, 12), TextFormat("PC: $%04X", cpu->pc));
        GuiLabel(layout_row(&status, 12), TextFormat("A: $%02X", cpu->a));
        GuiLabel(layout_row(&status, 12), TextFormat("X: $%02X", cpu->x));
        GuiLabel(layout_row(&status, 12), TextFormat("Y: $%02X", cpu->y));
        GuiLabel(layout_row(&status, 12), TextFormat("SP: $%02X", cpu->sp));

        GuiLabel(layout_row(&status, 12), TextFormat("Status: $%02X", cpu->sr));

        GuiLabel(layout_row(&status, 12), TextFormat("Addr: $%04X", cpu->addr_bus));
        GuiLabel(layout_row(&status, 12), TextFormat("Data: $%02X", cpu->data_bus));
        GuiLabel(layout_row(&status, 12),
                 TextFormat("Opcode: $%02X/%s", cpu->opcode, cpu->alu_label));
        GuiLabel(layout_row(&status, 12), TextFormat("R/W: %s", cpu->is_read ? "R" : "W"));

        // Render the right panel (Memory)
        Rectangle right_box = {left_w + 4, top_h, sw - left_w - 12, sh - top_h - 8};
        GuiGroupBox(right_box, "Memory Viewer");
        Layout mem = layout_box(right_box, 12);
        layout_row(&mem, 0);

        if (GuiButton(layout_next(&mem, 35, 25), "<<"))
            mem_page = (mem_page - 16) & 0xFF;
        if (GuiButton(layout_next(&mem, 30, 25), "<"))
            mem_page = (mem_page - 1) & 0xFF;
        GuiLabel(layout_next(&mem, 175, 25),
                 TextFormat("Page: $%02X00-$%02XFF", mem_page, mem_page));
        if (GuiButton(layout_next(&mem, 30, 25), ">"))
            mem_page = (mem_page + 1) & 0xFF;
        if (GuiButton(layout_next(&mem, 35, 25), ">>"))
            mem_page = (mem_page + 16) & 0xFF;
        if (GuiButton(layout_next(&mem, 50, 25), "ZP"))
            mem_page = 0x00;
        if (GuiButton(layout_next(&mem, 55, 25), "Stack"))
            mem_page = 0x01;
        if (GuiButton(layout_next(&mem, 50, 25), "Vec"))
            mem_page = 0xFF;

        // Hex grid rows
        for (int row = 0; row < 16; row++) {
            Rectangle r = layout_row(&mem, 14);
            uint16_t base = (uint16_t)((mem_page << 8) + row * 16);
            DrawText(TextFormat("$%04X:", base), (int)r.x, (int)r.y, 10, DARKBLUE);
            float col_w = (r.width - 70.0f) / 16.0f;
            for (int col = 0; col < 16; col++) {
                uint16_t addr = base + col;
                DrawText(TextFormat("%02X", cpu->mem[addr]), (int)(r.x + 70 + col * col_w),
                         (int)r.y, 10, (addr == cpu->pc) ? RED : DARKGRAY);
            }
        }

        EndDrawing();
    }

    CloseWindow();
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

    // Different handler for GUI
    if (cmd_args.is_gui) {
        GUI_handler(&cmd_args, &cpu);
        return 0;
    }

    CLI_handler(&cmd_args, &cpu);
    return 0;
}
