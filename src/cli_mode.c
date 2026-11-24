#include "cli_mode.h"
#include "cpu_addr.h"
#include "safe_wrappers.h"

#include "string.h"
#include <stdlib.h>
#include <strings.h>

void cli_run(const CmdArgs* cmd_args, Cpu* cpu) {
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
