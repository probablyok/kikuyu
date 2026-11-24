#include "raygui.h"

/// @brief Layout structure used for abstracting away pixel rectangle placements
typedef struct {
    float x, y, w, h; // Container bounds
    float cx, cy;     // Current cursor position
    float row_h, pad; // Current row height & spacing
} Layout;

Layout layout_box(Rectangle r, float pad);

void layout_newline(Layout* l);

Rectangle layout_next(Layout* l, float w, float h);

Rectangle layout_row(Layout* l, float h);
