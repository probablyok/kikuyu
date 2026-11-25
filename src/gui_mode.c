#include "gui_mode.h"

#include "cpu_addr.h"
#include "cpu_macros.h"
#include "layout_helper.h"
#include "raygui.h"
#include "raylib.h"

#include <stdbool.h>
#include <stdint.h>

/// @brief Draws the toolbar part of the GUI
/// @param bar The rectangle in which the toolbar is drawn
/// @param cpu The emulated 6502 CPU
/// @param running Flag for whether the emulation is currently running
/// @param cycles_per_frame Rate to tick emulaotr
/// @param total_cycles Total number of times emulator has been kicked before reset
static void draw_toolbar(Rectangle bar, Cpu* cpu, bool* running, float* cycles_per_frame,
                         uint64_t* total_cycles) {
    const float gap = 6.0f;

    GuiToggle(layout_cut_left(&bar, 72.0f, gap), *running ? "Pause" : "Run", running);

    if (*running)
        GuiDisable();
    if (GuiButton(layout_cut_left(&bar, 85.0f, gap), "Step Inst")) {
        int guard = 0;
        do {
            cpu_tick(cpu);
            (*total_cycles)++;
        } while (cpu->cur_step != cpu_fetch_opcode && ++guard < 16);
    }
    if (GuiButton(layout_cut_left(&bar, 88.0f, gap), "Step Cycle")) {
        cpu_tick(cpu);
        (*total_cycles)++;
    }
    if (*running)
        GuiEnable();

    if (GuiButton(layout_cut_left(&bar, 64.0f, gap), "Reset")) {
        *running = false;
        cpu_reset(cpu);
        *total_cycles = 0;
    }
    if (GuiButton(layout_cut_left(&bar, 48.0f, gap), "IRQ")) {
        cpu->irq_pending = true;
    }
    if (GuiButton(layout_cut_left(&bar, 48.0f, gap), "NMI")) {
        cpu->nmi_pending = true;
    }

    // Reserve right-side space for raygui's external slider text so it never clips
    Rectangle slider_label_slot = layout_cut_right(&bar, 58.0f, 4.0f);
    (void)slider_label_slot;
    GuiSliderBar(bar, "", TextFormat("%d cpf", (int)(*cycles_per_frame)), cycles_per_frame, 1.0f,
                 100.0f);
}

/// @brief Draws the status of each CPU register on GUI
/// @param box The rectangle in which the statuses are drawn
/// @param cpu The emulated 6502 CPU
static void draw_registers_panel(Rectangle box, const Cpu* cpu) {
    GuiGroupBox(box, "Registers");
    Rectangle inner = layout_group_inner(box, 10.0f);

    const int count = 5;
    float row_h = (inner.height - 4.0f * (float)(count - 1)) / (float)count;
    if (row_h > 24.0f)
        row_h = 24.0f;

    GuiLabel(layout_cut_top(&inner, row_h, 4.0f), TextFormat("PC: $%04X", cpu->pc));
    GuiLabel(layout_cut_top(&inner, row_h, 4.0f), TextFormat("A: $%02X", cpu->a));
    GuiLabel(layout_cut_top(&inner, row_h, 4.0f), TextFormat("X: $%02X", cpu->x));
    GuiLabel(layout_cut_top(&inner, row_h, 4.0f), TextFormat("Y: $%02X", cpu->y));
    GuiLabel(layout_cut_top(&inner, row_h, 4.0f), TextFormat("SP: $%02X", cpu->sp));
}

/// @brief Draws the status of each CPU bus on GUI
/// @param box The rectangle in which the statuses are drawn
/// @param cpu The emulated 6502 CPU
/// @param total_cycles Total number of times emulator has been kicked before reset
static void draw_buses_panel(Rectangle box, const Cpu* cpu) {
    GuiGroupBox(box, "Buses");
    Rectangle inner = layout_group_inner(box, 10.0f);

    const int count = 3;
    float row_h = (inner.height - 4.0f * (float)(count - 1)) / (float)count;
    if (row_h > 24.0f)
        row_h = 24.0f;

    GuiLabel(layout_cut_top(&inner, row_h, 4.0f), TextFormat("Addr: $%04X", cpu->addr_bus));
    GuiLabel(layout_cut_top(&inner, row_h, 4.0f), TextFormat("Data: $%02X", cpu->data_bus));
    GuiLabel(layout_cut_top(&inner, row_h, 4.0f), TextFormat("R/W: %s", cpu->is_read ? "R" : "W"));
}

/// @brief Draws the status of misc states on GUI
/// @param box The rectangle in which the statuses are drawn
/// @param cpu The emulated 6502 CPU
/// @param total_cycles Total number of times emulator has been kicked before reset
static void draw_states_panel(Rectangle box, const Cpu* cpu, uint64_t total_cycles) {
    GuiGroupBox(box, "States");
    Rectangle inner = layout_group_inner(box, 10.0f);

    const int count = 3;
    float row_h = (inner.height - 4.0f * (float)(count - 1)) / (float)count;
    if (row_h > 24.0f)
        row_h = 24.0f;

    const char* mnemonic = (cpu->alu_label[0] != '\0') ? cpu->alu_label : "---";

    GuiLabel(layout_cut_top(&inner, row_h, 4.0f),
             TextFormat("Opcode: $%02X (%s)", cpu->opcode, mnemonic));
    GuiLabel(layout_cut_top(&inner, row_h, 4.0f),
             TextFormat("IRQ/NMI: %d / %d", cpu->irq_pending ? 1 : 0, cpu->nmi_pending ? 1 : 0));
    GuiLabel(layout_cut_top(&inner, row_h, 4.0f),
             TextFormat("Cycles: %llu", (unsigned long long)total_cycles));
}

/// @brief Draws the memory viewer on the GUI
/// @param box The rectangle in which the memory viewer is drawn
/// @param cpu The emulated 6502 CPU
/// @param mem_page Reference to the current page address being viewed
static void draw_memory_panel(Rectangle box, const Cpu* cpu, int* mem_page) {
    GuiGroupBox(box, "Memory Viewer");
    Rectangle inner = layout_group_inner(box, 10.0f);

    // Top navigation strip
    Rectangle nav = layout_cut_top(&inner, 26.0f, 8.0f);
    const float gap = 4.0f;

    if (GuiButton(layout_cut_left(&nav, 32.0f, gap), "<<"))
        *mem_page = (*mem_page - 16) & 0xFF;
    if (GuiButton(layout_cut_left(&nav, 26.0f, gap), "<"))
        *mem_page = (*mem_page - 1) & 0xFF;

    // Quick-jump buttons anchored to the right
    if (GuiButton(layout_cut_right(&nav, 42.0f, gap), "Vec"))
        *mem_page = 0xFF;
    if (GuiButton(layout_cut_right(&nav, 48.0f, gap), "Stack"))
        *mem_page = 0x01;
    if (GuiButton(layout_cut_right(&nav, 36.0f, gap), "ZP"))
        *mem_page = 0x00;

    if (GuiButton(layout_cut_right(&nav, 32.0f, gap), ">>"))
        *mem_page = (*mem_page + 16) & 0xFF;
    if (GuiButton(layout_cut_right(&nav, 26.0f, gap), ">"))
        *mem_page = (*mem_page + 1) & 0xFF;

    // Remaining center space holds the current page label
    GuiLabel(nav, TextFormat("Page $%02X00-$%02XFF", *mem_page, *mem_page));

    // 16x16 memory dump rows filling the remaining vertical area
    float row_h = inner.height / 16.0f;
    uint16_t sp_addr = (uint16_t)(CPU_MEM_STACK_START + cpu->sp);

    for (int row = 0; row < 16; row++) {
        Rectangle r = layout_cut_top(&inner, row_h, 0.0f);
        Rectangle addr_col = layout_cut_left(&r, 56.0f, 6.0f);

        uint16_t base = (uint16_t)((*mem_page << 8) + row * 16);
        int text_y = (int)(r.y + (r.height - 10.0f) * 0.5f);

        DrawText(TextFormat("$%04X:", base), (int)addr_col.x, text_y, 10, DARKBLUE);

        for (int col = 0; col < 16; col++) {
            Rectangle byte_cell = layout_cell(r, col, 16, 2.0f);
            uint16_t addr = base + (uint16_t)col;
            Color color = DARKGRAY;
            if (addr == cpu->pc) {
                color = RED;
            } else if (addr == sp_addr) {
                color = BLUE;
            }
            DrawText(TextFormat("%02X", cpu->mem[addr]), (int)byte_cell.x, text_y, 10, color);
        }
    }
}

/// @brief Draws the status register of the CPU on the GUI
/// @param box The rectangle in which the status register is drawn
/// @param sr The emulated 6502 CPU's status register
static void draw_status_panel(Rectangle box, uint8_t sr) {
    static const char* flag_names[8] = {"N", "V", "U", "B", "D", "I", "Z", "C"};

    GuiGroupBox(box, "Status Register");
    Rectangle inner = layout_group_inner(box, 10.0f);

    // Top summary row: Hex value
    Rectangle hex_row = layout_cut_top(&inner, 20.0f, 6.0f);
    GuiLabel(hex_row, TextFormat("Hex: $%02X", sr));

    // Remaining area: 8-bit breakdown with bit indices (7..0) and flag names
    Rectangle header_row = layout_cut_top(&inner, 18.0f, 4.0f);
    Rectangle bit_row = inner;

    for (int col = 0; col < 8; col++) {
        int bit_idx = 7 - col;
        bool is_set = ((sr >> bit_idx) & 1U) != 0;

        Rectangle hdr_cell = layout_cell(header_row, col, 8, 6.0f);
        Rectangle val_cell = layout_cell(bit_row, col, 8, 6.0f);

        const char* hdr_text = TextFormat("%d:%s", bit_idx, flag_names[col]);
        int hdr_w = MeasureText(hdr_text, 10);
        DrawText(hdr_text, (int)(hdr_cell.x + (hdr_cell.width - (float)hdr_w) * 0.5f),
                 (int)(hdr_cell.y + 4.0f), 10, DARKGRAY);

        Color fill = is_set ? Fade(DARKBLUE, 0.85f) : Fade(LIGHTGRAY, 0.5f);
        Color border = is_set ? DARKBLUE : GRAY;
        Color fg = is_set ? RAYWHITE : DARKGRAY;

        DrawRectangleRec(val_cell, fill);
        DrawRectangleLinesEx(val_cell, 1.0f, border);

        const char* bit_text = is_set ? "1" : "0";
        int bit_w = MeasureText(bit_text, 14);
        DrawText(bit_text, (int)(val_cell.x + (val_cell.width - (float)bit_w) * 0.5f),
                 (int)(val_cell.y + (val_cell.height - 14.0f) * 0.5f), 10, fg);
    }
}

void gui_run(const CmdArgs* cmd_args, Cpu* cpu) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(820, 560, "Kikuyu 6502 Emulator");
    SetWindowMinSize(820, 560);
    SetTargetFPS(60);

    bool running = false;
    float cycles_per_frame = 1.0f;
    uint64_t total_cycles = 0;
    int mem_page = (int)(cmd_args->bin_offset >> 8) & 0xFF;

    while (!WindowShouldClose()) {
        if (running) {
            int steps = (int)cycles_per_frame;
            for (int i = 0; i < steps; i++) {
                cpu_tick(cpu);
                total_cycles++;
            }
        }

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        Rectangle root = layout_pad(
            (Rectangle){0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()}, 8.0f);

        // Top toolbar
        Rectangle top_bar = layout_cut_top(&root, 32.0f, 8.0f);
        draw_toolbar(top_bar, cpu, &running, &cycles_per_frame, &total_cycles);

        // Left (28%) and right (72%) columns
        Rectangle left_col = layout_cut_left(&root, 152.0f, 8.0f);
        Rectangle right_col = root;

        // Left column: Registers (top) + Buses (bottom)
        Rectangle misc_box = layout_cut_bottom(&left_col, 108.0f, 8.0f);
        Rectangle reg_box = layout_cut_top(&left_col, left_col.height * 0.6, 8.0f);
        Rectangle bus_box = left_col;
        draw_registers_panel(reg_box, cpu);
        draw_buses_panel(bus_box, cpu);
        draw_states_panel(misc_box, cpu, total_cycles);

        // Right column: Memory Viewer (top) + Status Register (bottom)
        Rectangle sr_box = layout_cut_bottom(&right_col, 108.0f, 8.0f);
        Rectangle mem_box = right_col;
        draw_memory_panel(mem_box, cpu, &mem_page);
        draw_status_panel(sr_box, cpu->sr);

        EndDrawing();
    }

    CloseWindow();
}
