#ifndef LAYOUT_HELPER_H
#define LAYOUT_HELPER_H

#include "raylib.h"

/// @brief Layout structure used for abstracting away pixel rectangle placements
typedef struct {
    Rectangle bounds;
    float x;
    float y;
    float padding;
    float row_height;
} Layout;

Layout layout_box(Rectangle bounds, float padding);
Rectangle layout_next(Layout* l, float w, float h);
Rectangle layout_row(Layout* l, float h);

Rectangle layout_pad(Rectangle r, float pad);
Rectangle layout_group_inner(Rectangle group_box, float pad);
Rectangle layout_cut_top(Rectangle* r, float height, float gap);
Rectangle layout_cut_bottom(Rectangle* r, float height, float gap);
Rectangle layout_cut_left(Rectangle* r, float width, float gap);
Rectangle layout_cut_right(Rectangle* r, float width, float gap);
Rectangle layout_cell(Rectangle row, int index, int count, float gap);

#endif // LAYOUT_HELPER_H
